/*
 * admin_handler.c
 *
 *  Created on: Apr 19, 2018
 *      Author: pchero
 */

#include <stdio.h>
#include <evhtp.h>
#include <jansson.h>

#include "slog.h"
#include "utils.h"

#include "user_handler.h"
#include "http_handler.h"

#include "admin_handler.h"

#define DEF_ADMIN_AUTHTOKEN_TYPE       "admin"


static json_t* get_users_all(const json_t* j_user);


bool admin_init_handler(void)
{
  return true;
}

bool admin_term_handler(void)
{
  return true;
}

bool admin_reload_handler(void)
{
  int ret;

  ret = admin_term_handler();
  if(ret == false) {
    slog(LOG_ERR, "Could not terminate admin_handler.");
    return false;
  }

  ret = admin_init_handler();
  if(ret == false) {
    slog(LOG_ERR, "Could not initiate admin_handler.");
    return false;
  }

  return true;
}

/**
 * POST ^/admin/login request handler.
 * @param req
 * @param data
 */
void admin_htp_post_admin_login(evhtp_request_t *req, void *data)
{
  json_t* j_res;
  json_t* j_tmp;
  char* username;
  char* password;
  char* authtoken;
  int ret;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired admin_htp_post_admin_login.");

  // get username/pass
  ret = http_get_htp_id_pass(req, &username, &password);
  if(ret == false) {
    sfree(username);
    sfree(password);
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  // create authtoken
  authtoken = user_create_authtoken(username, password, DEF_ADMIN_AUTHTOKEN_TYPE);
  sfree(username);
  sfree(password);
  if(authtoken == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  j_tmp = json_pack("{s:s}",
      "authtoken",  authtoken
      );
  sfree(authtoken);

  // create result
  j_res = http_create_default_result(EVHTP_RES_OK);
  json_object_set_new(j_res, "result", j_tmp);

  // response
  http_simple_response_normal(req, j_res);
  json_decref(j_res);

  return;
}

/**
 * GET ^/admin/users request handler.
 * @param req
 * @param data
 */
void admin_htp_get_admin_users(evhtp_request_t *req, void *data)
{
  json_t* j_user;
  json_t* j_res;
  json_t* j_tmp;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired admin_htp_get_admin_users.");

  j_user = http_get_userinfo(req);
  if(j_user == NULL) {
    slog(LOG_NOTICE, "Could not get correct userinfo.");
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // get info
  j_tmp = get_users_all(j_user);
  json_decref(j_user);
  if(j_tmp == NULL) {
    slog(LOG_NOTICE, "Could not get users info.");
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // create result
  j_res = http_create_default_result(EVHTP_RES_OK);
  json_object_set_new(j_res, "result", json_object());
  json_object_set_new(json_object_get(j_res, "result"), "list", j_tmp);

  // response
  http_simple_response_normal(req, j_res);
  json_decref(j_res);

  return;
}

static json_t* get_users_all(const json_t* j_user)
{
  json_t* j_res;

  j_res = user_get_userinfos_all();
  if(j_res == NULL) {
    return NULL;
  }

  return j_res;
}