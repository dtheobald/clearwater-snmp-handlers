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

/*
 *  Note: this file originally auto-generated by mib2c using
 *        : mib2c.array-user.conf 15997 2007-03-25 22:28:35Z dts12 $
 *
 *  $Id:$
 */

#include "itu_alarm_table.hpp"
#include "log.h"

static netsnmp_handler_registration* my_handler = NULL;
static netsnmp_table_array_callbacks cb;
/************************************************************
 *
 *  Initializes the ituAlarmTable module (not used in testing)
 */
// LCOV_EXCL_START
void init_ituAlarmTable(AlarmTableDefs& defs)
{
  if (initialize_table_ituAlarmTable() == SNMP_ERR_NOERROR)
  {
    ituAlarmTable_insert_defs(defs);
  }
}
// LCOV_EXCL_STOP

/************************************************************
 *
 * Inserts all of the alarm definitions
 */
void ituAlarmTable_insert_defs(AlarmTableDefs& defs)
{
  for (AlarmTableDefsIterator it = defs.begin(); it != defs.end(); it++)
  {
    ituAlarmTable_context* ctx = ituAlarmTable_create_row_context((char*) "", 
                                                                  it->alarm_index(), 
                                                                  it->severity());
    if (ctx)
    {
      ctx->_alarm_table_def = &(*it);

      CONTAINER_INSERT(cb.container, ctx);
    }
  }
}
/************************************************************
 *
 *  Initialize the ituAlarmTable table by defining its contents
 *  and how it's structured
 */
int initialize_table_ituAlarmTable(void)
{
  netsnmp_table_registration_info *table_info;

  if (my_handler)
  {
    // LCOV_EXCL_START
    snmp_log(LOG_ERR, "initialize_table_ituAlarmTable called again");
    return SNMP_ERR_NOERROR;
    // LCOV_EXCL_STOP
  }

  memset(&cb, 0x00, sizeof(cb));

  /** create the table structure itself */
  table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);

  my_handler = netsnmp_create_handler_registration("ituAlarmTable",
                                                   netsnmp_table_array_helper_handler,
                                                   ituAlarmTable_oid,
                                                   ituAlarmTable_oid_len,
                                                   HANDLER_CAN_RONLY);
            
  if (!my_handler || !table_info)
  {
    // LCOV_EXCL_START
    snmp_log(LOG_ERR, "malloc failed: initialize_table_ituAlarmTable");
    return SNMP_ERR_GENERR;
    // LCOV_EXCL_STOP
  }

  /*
   * Setting up the table's definition
   */

  /** index: alarmListName */
  netsnmp_table_helper_add_index(table_info, ASN_OCTET_STR);
  /** index: alarmModelIndex */
  netsnmp_table_helper_add_index(table_info, ASN_UNSIGNED);
  /** index: alarmModelState */
  netsnmp_table_helper_add_index(table_info, ASN_INTEGER);

  table_info->min_column = ituAlarmTable_COL_MIN;
  table_info->max_column = ituAlarmTable_COL_MAX;

  /*
   * registering the table with the master agent
   */
  cb.get_value = ituAlarmTable_get_value;
  cb.container = netsnmp_container_find("ituAlarmTable_primary:"
                                        "ituAlarmTable:"
                                        "table_container");
  cb.can_set = 0;

  DEBUGMSGTL(("initialize_table_ituAlarmTable", "Registering as table array\n"));

  netsnmp_table_container_register(my_handler, table_info, &cb, cb.container, 1);

  return SNMP_ERR_NOERROR;
}

/************************************************************
 *
 *  This routine is called for get requests to copy the data
 *  from the context to the varbind for the request.
 */
int ituAlarmTable_get_value(netsnmp_request_info* request,
                            netsnmp_index* item,
                            netsnmp_table_request_info* table_info)
{
  netsnmp_variable_list* var = request->requestvb;
  ituAlarmTable_context* context = (ituAlarmTable_context*) item;

  switch(table_info->colnum)
  {
    case COLUMN_ITUALARMEVENTTYPE:
    {
      static long event_type = IANAITUEVENTTYPE_OPERATIONALVIOLATION;
      snmp_set_var_typed_value(var, ASN_INTEGER,
                               (u_char*) &event_type,
                               sizeof(event_type));
    }
    break;
    
    case COLUMN_ITUALARMPROBABLECAUSE:
    {
      long cause = context->_alarm_table_def->cause();
      snmp_set_var_typed_value(var, ASN_INTEGER,
                               (u_char*) &cause,
                               sizeof(cause));
    }
    break;

    case COLUMN_ITUALARMADDITIONALTEXT:
    {
      snmp_set_var_typed_value(var, ASN_OCTET_STR,
                               (u_char*) context->_alarm_table_def->details().c_str(),
                               context->_alarm_table_def->details().length());
    }
    break;
    
    // LCOV_EXCL_START
    case COLUMN_ITUALARMGENERICMODEL:
    {
      snmp_set_var_typed_value(var, ASN_OBJECT_ID,
                               (u_char*) alarm_model_table_row_oid,
                               sizeof(alarm_model_table_row_oid));

      var->val.objid[ALARMMODELTABLEROW_INDEX] = context->_alarm_table_def->alarm_index();
      var->val.objid[ALARMMODELTABLEROW_STATE] = context->_alarm_table_def->state();
    }
    break;
    // LCOV_EXCL_STOP

    default: /** We shouldn't get here */
    {
      // LCOV_EXCL_START
      snmp_log(LOG_ERR, "unknown column: ituAlarmTable_get_value");
      return SNMP_ERR_GENERR;
      // LCOV_EXCL_STOP
    }
  }

  return SNMP_ERR_NOERROR;
}

/************************************************************
 * 
 *  Create a new row context and initialize its oid index.
 */
ituAlarmTable_context* ituAlarmTable_create_row_context(char* name,
                                                        unsigned long index,
                                                        long severity)
{
  ituAlarmTable_context* ctx = SNMP_MALLOC_TYPEDEF(ituAlarmTable_context);
  if (!ctx)
  {
    // LCOV_EXCL_START    
    snmp_log(LOG_ERR, "malloc failed: ituAlarmTable_create_row_context");
    return NULL;
    // LCOV_EXCL_STOP
  }
        
  if (ituAlarmTable_index_to_oid(name, index, severity, &ctx->_index) != SNMP_ERR_NOERROR)
  {
    // LCOV_EXCL_START
    if (ctx->_index.oids != NULL)
    {
      free(ctx->_index.oids);
    }

    free(ctx);
    return NULL;
    // LCOV_EXCL_STOP
  }

  return ctx;
}

/************************************************************
 *
 *  Convert table index components to an oid.
 */
int ituAlarmTable_index_to_oid(char* name,
                               unsigned long index,
                               long severity,
                               netsnmp_index *oid_idx)
{
  int err = SNMP_ERR_NOERROR;

  netsnmp_variable_list var_alarmListName;
  netsnmp_variable_list var_alarmModelIndex;
  netsnmp_variable_list var_alarmModelSeverity;

  /*
   * set up varbinds
   */
  memset(&var_alarmListName, 0x00, sizeof(var_alarmListName));
  var_alarmListName.type = ASN_OCTET_STR;
  memset(&var_alarmModelIndex, 0x00, sizeof(var_alarmModelIndex));
  var_alarmModelIndex.type = ASN_UNSIGNED;
  memset(&var_alarmModelSeverity, 0x00, sizeof(var_alarmModelSeverity));
  var_alarmModelSeverity.type = ASN_INTEGER;

  /*
   * chain index varbinds together
   */
  var_alarmListName.next_variable = &var_alarmModelIndex;
  var_alarmModelIndex.next_variable = &var_alarmModelSeverity;
  var_alarmModelSeverity.next_variable =  NULL;


  DEBUGMSGTL(("verbose:ituAlarmTable:ituAlarmTable_index_to_oid", "called\n"));

  snmp_set_var_value(&var_alarmListName, (u_char*) name, strlen(name));
  snmp_set_var_value(&var_alarmModelIndex, (u_char*) &index, sizeof(index));
  snmp_set_var_value(&var_alarmModelSeverity, (u_char*) &severity, sizeof(severity));

  err = build_oid(&oid_idx->oids, &oid_idx->len, NULL, 0, &var_alarmListName);
  if (err)
  {
    snmp_log(LOG_ERR, "error %d converting index to oid: ituAlarmTable_index_to_oid", err); // LCOV_EXCL_LINE
  }

  /*
   * parsing may have allocated memory. free it.
   */
  snmp_reset_var_buffers(&var_alarmListName);

  return err;
} 

