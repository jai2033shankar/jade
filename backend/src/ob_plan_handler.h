/*
 * plan_handler.h
 *
 *  Created on: Nov 30, 2015
 *    Author: pchero
 */

#ifndef SRC_PLAN_HANDLER_H_
#define SRC_PLAN_HANDLER_H_

#include <jansson.h>

typedef enum _E_DIAL_MODE {
  E_DIAL_MODE_NONE = 0,       /// No dial mode
  E_DIAL_MODE_PREDICTIVE = 1,   ///
  E_DIAL_MODE_DESKTOP = 2,    ///
  E_DIAL_MODE_POWER = 3,      ///
  E_DIAL_MODE_ROBO = 4,       ///
  E_DIAL_MODE_REDIRECT = 5,     ///
} E_DIAL_MODE;

/**
 * Determine when the dial list end.
 */
typedef enum _E_PLAN_DL_END_HANDLE {
  E_PLAN_DL_END_NONSTOP    = 0,  /// Keep current status.
  E_PLAN_DL_END_STOP       = 1,  /// Stop the campaign.
} E_PLAN_DL_END_HANDLE;


bool init_plan(void);

bool create_plan(const json_t* j_plan);
bool delete_plan(const char* uuid);
bool update_plan(const json_t* j_plan);
json_t* get_plan(const char* uuid);
json_t* get_plans_all(void);

json_t* create_dial_plan_info(json_t* j_plan);
bool is_endable_plan(json_t* j_plan);
bool is_nonstop_dl_handle(json_t* j_plan);

#endif /* SRC_PLAN_HANDLER_H_ */
