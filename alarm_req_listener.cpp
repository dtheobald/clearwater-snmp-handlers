/**
* Project Clearwater - IMS in the Cloud
* Copyright (C) 2014 Metaswitch Networks Ltd
*
* This program is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation, either version 3 of the License, or (at your
* option) any later version, along with the "Special Exception" for use of
* the program along with SSL, set forth below. This program is distributed
* in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR
* A PARTICULAR PURPOSE. See the GNU General Public License for more
* details. You should have received a copy of the GNU General Public
* License along with this program. If not, see
* <http://www.gnu.org/licenses/>.
*
* The author can be reached by email at clearwater@metaswitch.com or by
* post at Metaswitch Networks Ltd, 100 Church St, Enfield EN2 6BQ, UK
*
* Special Exception
* Metaswitch Networks Ltd grants you permission to copy, modify,
* propagate, and distribute a work formed by combining OpenSSL with The
* Software, or a work derivative of such a combination, even if such
* copying, modification, propagation, or distribution would otherwise
* violate the terms of the GPL. You must comply with the GPL in all
* respects for all of the code used other than OpenSSL.
* "OpenSSL" means OpenSSL toolkit software distributed by the OpenSSL
* Project and licensed under the OpenSSL Licenses, or a work based on such
* software and licensed under the OpenSSL Licenses.
* "OpenSSL Licenses" means the OpenSSL License and Original SSLeay License
* under which the OpenSSL Project distributes the OpenSSL toolkit software,
* as those licenses appear in the file LICENSE-OPENSSL.
*/

#include <string.h>
#include <zmq.h>

#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include "log.h"
#include "alarm_req_listener.hpp"
#include "alarm_trap_sender.hpp"

AlarmReqListener AlarmReqListener::_instance;

bool AlarmReqListener::start()
{
  if (!zmq_init_ctx())
  {
    return false;
  }

  pthread_mutex_init(&_start_mutex, NULL);
  pthread_cond_init(&_start_cond, NULL);

  pthread_mutex_lock(&_start_mutex);

  int rc = pthread_create(&_thread, NULL, &listener_thread, (void*) &_instance);
  if (rc == 0)
  {
    pthread_cond_wait(&_start_cond, &_start_mutex);
  }

  pthread_mutex_unlock(&_start_mutex);

  pthread_cond_destroy(&_start_cond);
  pthread_mutex_destroy(&_start_mutex);

  if (rc != 0)
  {
    // LCOV_EXCL_START - No mock for pthread_create
    TRC_ERROR("error creating listener thread: %s", strerror(rc));
    zmq_clean_ctx();
    return false;
    // LCOV_EXCL_STOP
  }

  return true;
}

void AlarmReqListener::stop()
{
  zmq_clean_ctx();

  pthread_join(_thread, NULL);
}

void* AlarmReqListener::listener_thread(void* alarm_req_listener)
{
  ((AlarmReqListener*) alarm_req_listener)->listener();

  return NULL;
}

AlarmReqListener::AlarmReqListener() : _ctx(NULL), _sck(NULL)
{
}

bool AlarmReqListener::zmq_init_ctx()
{
  _ctx = zmq_ctx_new();
  if (_ctx == NULL)
  {
    TRC_ERROR("zmq_ctx_new failed: %s", zmq_strerror(errno));
    return false;
  }

  return true;
}

bool AlarmReqListener::zmq_init_sck()
{
  _sck = zmq_socket(_ctx, ZMQ_REP);

  if (_sck == NULL)
  {
    TRC_ERROR("zmq_socket failed: %s", zmq_strerror(errno));
    return false;
  }

  // Set a LINGER period of 0 so that we exit immediately on shutdown
  int linger = 0;
  zmq_setsockopt(_sck, ZMQ_LINGER, &linger, sizeof(linger));

  std::string sck_file = std::string("/var/run/clearwater/alarms");
  std::string sck_url = std::string("ipc://" + sck_file);
  TRC_INFO("AlarmReqListener: ss='%s'", sck_url.c_str());

  int rc = -1, retry_cnt = 0, max_retry = 10;

#ifdef UNIT_TEST
  max_retry = 1;
#endif

  for (retry_cnt=0; retry_cnt < max_retry && rc == -1; retry_cnt++ )
  {
    if (retry_cnt > 0)
    {
      // LCOV_EXCL_START
      sleep(1);// Give it another shot after delay
      // LCOV_EXCL_STOP
    }

    // First try to remove the socket file
    rc = remove(sck_file.c_str());
    if (rc == -1)
    {
      if (errno != ENOENT)
      {
        // LCOV_EXCL_START
        TRC_ERROR("remove(%s) failed: %s", sck_file.c_str(), strerror(errno));
        continue;
        // LCOV_EXCL_STOP
      }
      TRC_ERROR("remove(%s) ignored: %s", sck_file.c_str(), strerror(errno));
    }

    // Any old file has been cleaned up, so try to bind
    while (((rc = zmq_bind(_sck, sck_url.c_str())) == -1) && (errno == EINTR))
    {
      // Ignore possible errors due to a syscall being interrupted by a signal.
    }
    if (rc == -1)
    {
      TRC_ERROR("zmq_bind failed: %s", zmq_strerror(errno));
      continue;
    }
    else
    {
      break;
    }

#ifndef UNIT_TEST
    rc=chmod(sck_file.c_str(), 0777);
    if (rc == -1)
    {
      TRC_ERROR("chmod(%s, 0777) failed: %s", sck_file.c_str(), strerror(errno));
      zmq_unbind(_sck, sck_url.c_str());
    }
#endif
  }

  if (rc == -1)
  {
    // LCOV_EXCL_START
#ifndef UNIT_TEST
    TRC_ERROR("zmq failed to bind %d times - killing myself", retry_cnt);
    kill(getpid(), SIGKILL);
#else
    TRC_ERROR("zmq failed to bind %d times", retry_cnt);
#endif
    // LCOV_EXCL_STOP
    return false;
  }
  
  TRC_ERROR("zmq socket initialized after %d tries", retry_cnt + 1);

  return true;
}

void AlarmReqListener::zmq_clean_ctx()
{
  if (_ctx)
  {
    if (zmq_ctx_destroy(_ctx) == -1)
    {
      TRC_ERROR("zmq_ctx_destroy failed: %s", zmq_strerror(errno));
    }

    _ctx = NULL;
  }
}

void AlarmReqListener::zmq_clean_sck()
{
  if (_sck)
  {
    if (zmq_close(_sck) == -1)
    {
      TRC_ERROR("zmq_close failed: %s", zmq_strerror(errno));
    }

    _sck = NULL;
  }
}

void AlarmReqListener::listener()
{
  bool sckOk = zmq_init_sck();

  pthread_mutex_lock(&_start_mutex);
  pthread_cond_signal(&_start_cond);
  pthread_mutex_unlock(&_start_mutex);

  if (!sckOk)
  {
    return;
  }

  while (1)
  {
    std::vector<std::string> msg;

    if (!next_msg(msg))
    {
      zmq_clean_sck();
      return;
    }

    if ((msg[0].compare("issue-alarm") == 0) && (msg.size() == 3))
    {
      AlarmTrapSender::get_instance().issue_alarm(msg[1], msg[2]);
    }
    else if ((msg[0].compare("clear-alarms") == 0) && (msg.size() == 2))
    {
      AlarmTrapSender::get_instance().clear_alarms(msg[1]);
    }
    else if ((msg[0].compare("sync-alarms") == 0) && (msg.size() == 1))
    {
      AlarmTrapSender::get_instance().sync_alarms(true);
    }
    else if ((msg[0].compare("sync-alarms-no-clear") == 0) && (msg.size() == 1))
    {
      AlarmTrapSender::get_instance().sync_alarms(false);
    }
    else
    {
      TRC_ERROR("unexpected alarm request: %s, %lu", msg[0].c_str(), msg.size());
    }

    reply("ok");
  }
}

bool AlarmReqListener::next_msg(std::vector<std::string>& msg)
{
  int rc;

  int more = 0;
  size_t more_sz = sizeof(more);

  do
  {
    zmq_msg_t msg_part;

    if (zmq_msg_init(&msg_part) != 0)
    {
      TRC_ERROR("zmq_msg_init failed: %s", zmq_strerror(errno));
      return false;
    }

    while (((rc = zmq_msg_recv(&msg_part, _sck, 0)) == -1) && (errno == EINTR))
    {
      // Ignore possible errors due to a syscall being interrupted by a signal.
    }

    if (rc == -1)
    {
      if (errno != ETERM)
      {
        TRC_ERROR("zmq_msg_recv failed: %s", zmq_strerror(errno));
      }

      return false;
    }

    msg.push_back(std::string((char*)zmq_msg_data(&msg_part), zmq_msg_size(&msg_part)));

    while (((rc = zmq_getsockopt(_sck, ZMQ_RCVMORE, &more, &more_sz)) == -1) && (errno == EINTR))
    {
      // Ignore possible errors due to a syscall being interrupted by a signal.
    }

    if (rc == -1)
    {
      if (errno != ETERM)
      {
        TRC_ERROR("zmq_getsockopt failed: %s", zmq_strerror(errno));
      }

      return false;
    }

    if (zmq_msg_close(&msg_part) == -1)
    {
      TRC_ERROR("zmq_msg_close failed: %s", zmq_strerror(errno));
      return false;
    }

  } while(more);

  return true;
}

void AlarmReqListener::reply(const char* response)
{
  if (zmq_send(_sck, response, strlen(response), 0) == -1)
  {
    TRC_ERROR("zmq_send failed: %s", zmq_strerror(errno));
  }
}

