/*
 * ob_http_handler.c
 *
 *  Created on: Mar 15, 2017
 *      Author: pchero
 */


#define _GNU_SOURCE

#include <jansson.h>
#include <stdbool.h>
#include <event2/event.h>
#include <evhtp.h>
#include <string.h>

#include "common.h"
#include "slog.h"
#include "utils.h"
#include "http_handler.h"
#include "ob_http_handler.h"
#include "resource_handler.h"

#include "ob_campaign_handler.h"
#include "ob_destination_handler.h"
#include "ob_dialing_handler.h"
#include "ob_dl_handler.h"
#include "ob_plan_handler.h"
#include "ob_dlma_handler.h"

extern evhtp_t* g_htps;

///// outbound modules


// ob/destinations
static void htp_get_ob_destinations(evhtp_request_t *req, void *data);
static void htp_post_ob_destinations(evhtp_request_t *req, void *data);
static void htp_get_ob_destinations_all(evhtp_request_t *req, void *data);
static void htp_get_ob_destinations_uuid(evhtp_request_t *req, void *data);
static void htp_put_ob_destinations_uuid(evhtp_request_t *req, void *data);
static void htp_delete_ob_destinations_uuid(evhtp_request_t *req, void *data);

// ob/plans
static void htp_get_ob_plans(evhtp_request_t *req, void *data);
static void htp_post_ob_plans(evhtp_request_t *req, void *data);
static void htp_get_ob_plans_all(evhtp_request_t *req, void *data);
static void htp_get_ob_plans_uuid(evhtp_request_t *req, void *data);
static void htp_put_ob_plans_uuid(evhtp_request_t *req, void *data);
static void htp_delete_ob_plans_uuid(evhtp_request_t *req, void *data);

// ob/campaigns
static void htp_get_ob_campaigns(evhtp_request_t *req, void *data);
static void htp_post_ob_campaigns(evhtp_request_t *req, void *data);
static void htp_get_ob_campaigns_all(evhtp_request_t *req, void *data);
static void htp_get_ob_campaigns_uuid(evhtp_request_t *req, void *data);
static void htp_put_ob_campaigns_uuid(evhtp_request_t *req, void *data);
static void htp_delete_ob_campaigns_uuid(evhtp_request_t *req, void *data);

// ob/dlmas
static void htp_get_ob_dlmas(evhtp_request_t *req, void *data);
static void htp_post_ob_dlmas(evhtp_request_t *req, void *data);
static void htp_get_ob_dlmas_all(evhtp_request_t *req, void *data);
static void htp_get_ob_dlmas_uuid(evhtp_request_t *req, void *data);
static void htp_put_ob_dlmas_uuid(evhtp_request_t *req, void *data);
static void htp_delete_ob_dlmas_uuid(evhtp_request_t *req, void *data);

// ob/dls
static void htp_get_ob_dls(evhtp_request_t *req, void *data);
static void htp_post_ob_dls(evhtp_request_t *req, void *data);
static void htp_get_ob_dls_all(evhtp_request_t *req, void *data);
static void htp_get_ob_dls_uuid(evhtp_request_t *req, void *data);
static void htp_put_ob_dls_uuid(evhtp_request_t *req, void *data);
static void htp_delete_ob_dls_uuid(evhtp_request_t *req, void *data);

// ob/dialings
static void htp_get_ob_dialings(evhtp_request_t *req, void *data);
static void htp_get_ob_dialings_all(evhtp_request_t *req, void *data);
static void htp_get_ob_dialings_uuid(evhtp_request_t *req, void *data);
static void htp_delete_ob_dialings_uuid(evhtp_request_t *req, void *data);

/**
 * http request handler.
 * request : ^/ob/destinations
 * @param req
 * @param data
 */
void ob_cb_htp_ob_destinations(evhtp_request_t *req, void *data)
{
  int method;
  int ret;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired cb_htp_ob_destinations.");

  // check authorization
  ret = http_is_request_has_permission(req, EN_HTTP_PERM_ADMIN);
  if(ret == false) {
    http_simple_response_error(req, EVHTP_RES_FORBIDDEN, 0, NULL);
    return;
  }

  // method check
  method = evhtp_request_get_method(req);
  if((method != htp_method_GET) && (method != htp_method_POST)) {
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
    return;
  }

  if(method == htp_method_GET) {
    htp_get_ob_destinations(req, data);
    return;
  }
  else if(method == htp_method_POST) {
    htp_post_ob_destinations(req, data);
    return;
  }
  else {
    // should not reach to here.
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
  }

  // should not reach to here.
  http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);

  return;
}

/**
 * http request handler.
 * request : ^/ob/destinations/
 * @param req
 * @param data
 */
void ob_cb_htp_ob_destinations_all(evhtp_request_t *req, void *data)
{
  int method;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired cb_htp_ob_destinations_all.");

  // method check
  method = evhtp_request_get_method(req);
  if(method != htp_method_GET) {
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
    return;
  }

  if(method == htp_method_GET) {
    htp_get_ob_destinations_all(req, data);
    return;
  }
  else {
    // should not reach to here.
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
    return;
  }

  // should not reach to here.
  http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);

  return;
}

/**
 * http request handler.
 * request : ^/ob/destinations/<uuid>
 * @param req
 * @param data
 */
void ob_cb_htp_ob_destinations_detail(evhtp_request_t *req, void *data)
{
  int method;
  int ret;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired cb_htp_ob_destinations_uuid.");

  // check authorization
  ret = http_is_request_has_permission(req, EN_HTTP_PERM_ADMIN);
  if(ret == false) {
    http_simple_response_error(req, EVHTP_RES_FORBIDDEN, 0, NULL);
    return;
  }

  // method check
  method = evhtp_request_get_method(req);
  if((method != htp_method_GET) && (method != htp_method_PUT) && (method != htp_method_DELETE)) {
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
    return;
  }

  if(method == htp_method_GET) {
    htp_get_ob_destinations_uuid(req, data);
    return;
  }
  else if(method == htp_method_PUT) {
    htp_put_ob_destinations_uuid(req, data);
    return;
  }
  else if(method == htp_method_DELETE) {
    htp_delete_ob_destinations_uuid(req, data);
    return;
  }
  else {
    // should not reach to here.
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
    return;
  }

  // should not reach to here.
  http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);

  return;
}

/**
 * http request handler.
 * request : ^/ob/plans
 * @param req
 * @param data
 */
void ob_cb_htp_ob_plans(evhtp_request_t *req, void *data)
{
  int method;
  int ret;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired cb_htp_ob_plans.");

  // check authorization
  ret = http_is_request_has_permission(req, EN_HTTP_PERM_ADMIN);
  if(ret == false) {
    http_simple_response_error(req, EVHTP_RES_FORBIDDEN, 0, NULL);
    return;
  }

  // method check
  method = evhtp_request_get_method(req);
  if((method != htp_method_GET) && (method != htp_method_POST)) {
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
    return;
  }

  if(method == htp_method_GET) {
    htp_get_ob_plans(req, data);
    return;
  }
  else if(method == htp_method_POST) {
    htp_post_ob_plans(req, data);
    return;
  }
  else {
    // should not reach to here.
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
  }

  // should not reach to here.
  http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);

  return;
}

/**
 * http request handler.
 * request : ^/ob/plans/
 * @param req
 * @param data
 */
void ob_cb_htp_ob_plans_all(evhtp_request_t *req, void *data)
{
  int method;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired cb_htp_ob_plans_all.");

  // method check
  method = evhtp_request_get_method(req);
  if(method != htp_method_GET) {
    slog(LOG_ERR, "Wrong method request. method[%d]", method);
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
    return;
  }

  if(method == htp_method_GET) {
    htp_get_ob_plans_all(req, data);
    return;
  }
  else {
    // should not reach to here.
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
    return;
  }

  // should not reach to here.
  http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);

  return;
}

/**
 * http request handler.
 * request : ^/ob/plans/<uuid>
 * @param req
 * @param data
 */
void ob_cb_htp_ob_plans_detail(evhtp_request_t *req, void *data)
{
  int method;
  int ret;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired cb_htp_ob_plans_uuid.");

  // check authorization
  ret = http_is_request_has_permission(req, EN_HTTP_PERM_ADMIN);
  if(ret == false) {
    http_simple_response_error(req, EVHTP_RES_FORBIDDEN, 0, NULL);
    return;
  }

  // method check
  method = evhtp_request_get_method(req);
  if((method != htp_method_GET) && (method != htp_method_PUT) && (method != htp_method_DELETE)) {
    slog(LOG_ERR, "Wrong method request. method[%d]", method);
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
    return;
  }

  if(method == htp_method_GET) {
    htp_get_ob_plans_uuid(req, data);
    return;
  }
  else if(method == htp_method_PUT) {
    htp_put_ob_plans_uuid(req, data);
    return;
  }
  else if(method == htp_method_DELETE) {
    htp_delete_ob_plans_uuid(req, data);
    return;
  }
  else {
    // should not reach to here.
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
    return;
  }

  // should not reach to here.
  http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);

  return;
}

/**
 * http request handler.
 * request : ^/ob/campaigns
 * @param req
 * @param data
 */
void ob_cb_htp_ob_campaigns(evhtp_request_t *req, void *data)
{
  int method;
  int ret;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired cb_htp_ob_campaigns.");

  // check authorization
  ret = http_is_request_has_permission(req, EN_HTTP_PERM_ADMIN);
  if(ret == false) {
    http_simple_response_error(req, EVHTP_RES_FORBIDDEN, 0, NULL);
    return;
  }

  // method check
  method = evhtp_request_get_method(req);
  if((method != htp_method_GET) && (method != htp_method_POST)) {
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
    return;
  }

  if(method == htp_method_GET) {
    htp_get_ob_campaigns(req, data);
    return;
  }
  else if(method == htp_method_POST) {
    htp_post_ob_campaigns(req, data);
    return;
  }
  else {
    // should not reach to here.
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
  }

  // should not reach to here.
  http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);

  return;
}

/**
 * http request handler.
 * request : ^/ob/campaigns/<uuid>
 * @param req
 * @param data
 */
void ob_cb_htp_ob_campaigns_detail(evhtp_request_t *req, void *data)
{
  int method;
  const char* uuid;
  int ret;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired cb_htp_ob_campaigns_uuid.");

  // check authorization
  ret = http_is_request_has_permission(req, EN_HTTP_PERM_ADMIN);
  if(ret == false) {
    http_simple_response_error(req, EVHTP_RES_FORBIDDEN, 0, NULL);
    return;
  }

  // method check
  method = evhtp_request_get_method(req);
  if((method != htp_method_GET) && (method != htp_method_PUT) && (method != htp_method_DELETE)) {
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
    return;
  }

  // get uuid
  uuid = req->uri->path->match_start;
  if(uuid == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  if(method == htp_method_GET) {
    htp_get_ob_campaigns_uuid(req, data);
    return;
  }
  else if(method == htp_method_PUT) {
    htp_put_ob_campaigns_uuid(req, data);
    return;
  }
  else if(method == htp_method_DELETE) {
    htp_delete_ob_campaigns_uuid(req, data);
    return;
  }
  else {
    // should not reach to here.
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
    return;
  }

  // should not reach to here.
  http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);

  return;
}

/**
 * http request handler.
 * request : ^/ob/campaigns/
 * @param req
 * @param data
 */
void ob_cb_htp_ob_campaigns_all(evhtp_request_t *req, void *data)
{
  int method;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired cb_htp_ob_campaigns_all.");

  // method check
  method = evhtp_request_get_method(req);
  if(method != htp_method_GET) {
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
    return;
  }

  if(method == htp_method_GET) {
    htp_get_ob_campaigns_all(req, data);
    return;
  }
  else {
    // should not reach to here.
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
    return;
  }

  // should not reach to here.
  http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);

  return;
}

/**
 * http request handler.
 * request : ^/ob/dlmas
 * @param req
 * @param data
 */
void ob_cb_htp_ob_dlmas(evhtp_request_t *req, void *data)
{
  int method;
  int ret;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired cb_htp_ob_dlmas.");

  // check authorization
  ret = http_is_request_has_permission(req, EN_HTTP_PERM_ADMIN);
  if(ret == false) {
    http_simple_response_error(req, EVHTP_RES_FORBIDDEN, 0, NULL);
    return;
  }

  // method check
  method = evhtp_request_get_method(req);
  if((method != htp_method_GET) && (method != htp_method_POST)) {
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
    return;
  }

  if(method == htp_method_GET) {
    htp_get_ob_dlmas(req, data);
    return;
  }
  else if(method == htp_method_POST) {
    htp_post_ob_dlmas(req, data);
    return;
  }
  else {
    // should not reach to here.
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
  }

  // should not reach to here.
  http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);

  return;
}

/**
 * http request handler.
 * request : ^/ob/dlmas/
 * @param req
 * @param data
 */
void ob_cb_htp_ob_dlmas_all(evhtp_request_t *req, void *data)
{
  int method;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired cb_htp_ob_dlmas_all.");

  // method check
  method = evhtp_request_get_method(req);
  if(method != htp_method_GET) {
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
    return;
  }

  if(method == htp_method_GET) {
    htp_get_ob_dlmas_all(req, data);
    return;
  }
  else {
    // should not reach to here.
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
  }

  // should not reach to here.
  http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);

  return;
}

/**
 * http request handler.
 * request : ^/ob/dlmas/<uuid>
 * @param req
 * @param data
 */
void ob_cb_htp_ob_dlmas_detail(evhtp_request_t *req, void *data)
{
  int method;
  const char* uuid;
  int ret;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired cb_htp_ob_dlmas_uuid.");

  // check authorization
  ret = http_is_request_has_permission(req, EN_HTTP_PERM_ADMIN);
  if(ret == false) {
    http_simple_response_error(req, EVHTP_RES_FORBIDDEN, 0, NULL);
    return;
  }

  // method check
  method = evhtp_request_get_method(req);
  if((method != htp_method_GET) && (method != htp_method_PUT) && (method != htp_method_DELETE)) {
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
    return;
  }

  // get uuid
  uuid = req->uri->path->match_start;
  if(uuid == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  if(method == htp_method_GET) {
    htp_get_ob_dlmas_uuid(req, data);
    return;
  }
  else if(method == htp_method_PUT) {
    htp_put_ob_dlmas_uuid(req, data);
    return;
  }
  else if(method == htp_method_DELETE) {
    htp_delete_ob_dlmas_uuid(req, data);
    return;
  }
  else {
    // should not reach to here.
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
    return;
  }

  // should not reach to here.
  http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);

  return;
}

/**
 * http request handler.
 * request : ^/ob/dls/
 * @param req
 * @param data
 */
void ob_cb_htp_ob_dls_all(evhtp_request_t *req, void *data)
{
  int method;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired cb_htp_ob_dls_all.");

  // method check
  method = evhtp_request_get_method(req);
  if(method != htp_method_GET) {
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
    return;
  }

  if(method == htp_method_GET) {
    htp_get_ob_dls_all(req, data);
    return;
  }
  else {
    // should not reach to here.
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
  }

  // should not reach to here.
  http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);

  return;
}

/**
 * http request handler.
 * request : ^/ob/dls
 * @param req
 * @param data
 */
void ob_cb_htp_ob_dls(evhtp_request_t *req, void *data)
{
  int method;
  int ret;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired cb_htp_ob_dls.");

  // check authorization
  ret = http_is_request_has_permission(req, EN_HTTP_PERM_ADMIN);
  if(ret == false) {
    http_simple_response_error(req, EVHTP_RES_FORBIDDEN, 0, NULL);
    return;
  }

  // method check
  method = evhtp_request_get_method(req);
  if((method != htp_method_GET) && (method != htp_method_POST)) {
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
    return;
  }

  if(method == htp_method_GET) {
    htp_get_ob_dls(req, data);
    return;
  }
  else if(method == htp_method_POST) {
    htp_post_ob_dls(req, data);
    return;
  }
  else {
    // should not reach to here.
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
  }

  // should not reach to here.
  http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);

  return;
}

/**
 * http request handler.
 * request : ^/ob/dls/<uuid>
 * @param req
 * @param data
 */
void ob_cb_htp_ob_dls_detail(evhtp_request_t *req, void *data)
{
  int method;
  const char* uuid;
  int ret;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired cb_htp_ob_dls_uuid.");

  // check authorization
  ret = http_is_request_has_permission(req, EN_HTTP_PERM_ADMIN);
  if(ret == false) {
    http_simple_response_error(req, EVHTP_RES_FORBIDDEN, 0, NULL);
    return;
  }

  // method check
  method = evhtp_request_get_method(req);
  if((method != htp_method_GET) && (method != htp_method_PUT) && (method != htp_method_DELETE)) {
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
    return;
  }

  // get uuid
  uuid = req->uri->path->match_start;
  if(uuid == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  if(method == htp_method_GET) {
    htp_get_ob_dls_uuid(req, data);
    return;
  }
  else if(method == htp_method_PUT) {
    htp_put_ob_dls_uuid(req, data);
    return;
  }
  else if(method == htp_method_DELETE) {
    htp_delete_ob_dls_uuid(req, data);
    return;
  }
  else {
    // should not reach to here.
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
    return;
  }

  // should not reach to here.
  http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);

  return;
}

/**
 * htp request handler.
 * request: GET ^/ob/destinations
 * @param req
 * @param data
 */
static void htp_get_ob_destinations(evhtp_request_t *req, void *data)
{
  json_t* j_tmp;
  json_t* j_res;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_get_ob_destinations.");

  // get info
  j_tmp = ob_get_destinations_all();
  if(j_tmp == NULL) {
    http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
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

/**
 * htp request handler.
 * request: POST ^/ob/destinations
 * @param req
 * @param data
 */
static void htp_post_ob_destinations(evhtp_request_t *req, void *data)
{
  const char* tmp_const;
  char* tmp;
  int ret;
  json_t* j_data;
  json_t* j_tmp;
  json_t* j_res;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_post_ob_destinations.");

  // get data
  tmp_const = (char*)evbuffer_pullup(req->buffer_in, evbuffer_get_length(req->buffer_in));
  if(tmp_const == NULL) {
    slog(LOG_ERR, "Could not get data from request.");
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  // create json
  tmp = strndup(tmp_const, evbuffer_get_length(req->buffer_in));
  slog(LOG_DEBUG, "Requested data. data[%s]", tmp);
  j_data = json_loads(tmp, JSON_DECODE_ANY, NULL);
  sfree(tmp);
  if(j_data == NULL) {
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  // validate data
  ret = ob_validate_destination(j_data);
  if(ret == false) {
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  // create destination
  j_tmp = ob_create_destination(j_data);
  json_decref(j_data);
  if(j_tmp == NULL) {
    slog(LOG_INFO, "Could not create ob_destination.");
    http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
    return;
  }

  // create result
  j_res = http_create_default_result(EVHTP_RES_OK);
  json_object_set_new(j_res, "result", j_tmp);

  // response
  http_simple_response_normal(req, j_res);
  json_decref(j_res);

  return;
}

/**
 * htp request handler.
 * request: GET ^/ob/destinations/
 * @param req
 * @param data
 */
static void htp_get_ob_destinations_all(evhtp_request_t *req, void *data)
{
  json_t* j_tmp;
  json_t* j_res;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_get_ob_destinations_all.");

  // get info
  j_tmp = ob_get_destinations_all();
  if(j_tmp == NULL) {
    http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
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

/**
 * htp request handler.
 * request: GET /destinations/<uuid>
 * @param req
 * @param data
 */
static void htp_get_ob_destinations_uuid(evhtp_request_t *req, void *data)
{
  char* detail;
  json_t* j_tmp;
  json_t* j_res;
  int ret;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }

  // get detail
  detail = http_get_parsed_detail(req);
  if(detail == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // check existence
  ret = ob_is_exist_destination(detail);
  if(ret == false) {
    sfree(detail);
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // get destination info
  j_tmp = ob_get_destination(detail);
  sfree(detail);
  if(j_tmp == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // create result
  j_res = http_create_default_result(EVHTP_RES_OK);
  json_object_set_new(j_res, "result", j_tmp);

  // response
  http_simple_response_normal(req, j_res);
  json_decref(j_res);

  return;
}

/**
 * htp request handler.
 * request: PUT ^/ob/destinations/<uuid>
 * @param req
 * @param data
 */
static void htp_put_ob_destinations_uuid(evhtp_request_t *req, void *data)
{
  char* detail;
  json_t* j_data;
  json_t* j_tmp;
  json_t* j_res;
  int ret;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_put_ob_destinations_uuid");

  // get detail
  detail = http_get_parsed_detail(req);
  if(detail == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  j_data = http_get_json_from_request_data(req);
  if(j_data == NULL) {
    sfree(detail);
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  // check existence
  ret = ob_is_exist_destination(detail);
  if(ret == false) {
    sfree(detail);
    json_decref(j_data);
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // update info
  json_object_set_new(j_data, "uuid", json_string(detail));
  sfree(detail);
  j_tmp = ob_update_destination(j_data);
  json_decref(j_data);
  if(j_tmp == NULL) {
    http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
    return;
  }

  // create result
  j_res = http_create_default_result(EVHTP_RES_OK);
  json_object_set_new(j_res, "result", j_tmp);

  // response
  http_simple_response_normal(req, j_res);
  json_decref(j_res);

  return;
}

/**
 * htp request handler.
 * request: DELETE ^/ob/destinations/<uuid>
 * @param req
 * @param data
 */
static void htp_delete_ob_destinations_uuid(evhtp_request_t *req, void *data)
{
  char* detail;
  const char* tmp_const;
  json_t* j_tmp;
  json_t* j_res;
  int ret;
  int force;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_INFO, "Fired htp_delete_ob_destinations_uuid.");

  // get uuid
  detail = http_get_parsed_detail(req);
  if(detail == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // check existence
  ret = ob_is_exist_destination(detail);
  if(ret == false) {
    slog(LOG_NOTICE, "Could not find correct ob_destination info. uuid[%s]", detail);
    sfree(detail);
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // get parameters
  force = 0;
  tmp_const = evhtp_kv_find(req->uri->query, "force");
  if(tmp_const != NULL) {
    force = atoi(tmp_const);
  }
  slog(LOG_DEBUG, "Check force option. force[%d]", force);

  // check force option
  if(force == 1) {
    ret = ob_clear_campaign_destination(detail);
    if(ret == false) {
      slog(LOG_ERR, "Could not clear destination info from campaign. dest_uuid[%s]", detail);
      sfree(detail);
      http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
      return;
    }
  }

  // check deletable
  ret = ob_is_deletable_destination(detail);
  if(ret == false) {
    slog(LOG_NOTICE, "The given destination info is not deletable. uuid[%s]", detail);
    sfree(detail);
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  // delete info
  j_tmp = ob_delete_destination(detail);
  sfree(detail);
  if(j_tmp == NULL) {
    slog(LOG_ERR, "Could not delete destination info.");
    http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
    return;
  }

  // create result
  j_res = http_create_default_result(EVHTP_RES_OK);
  json_object_set_new(j_res, "result", j_tmp);

  // response
  http_simple_response_normal(req, j_res);
  json_decref(j_res);

  return;
}

/**
 * htp request handler.
 * request: GET ^/ob/plans
 * @param req
 * @param data
 */
static void htp_get_ob_plans(evhtp_request_t *req, void *data)
{
  json_t* j_tmp;
  json_t* j_res;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_get_ob_plans.");

  // get info
  j_tmp = ob_get_plans_all();
  if(j_tmp == NULL) {
    http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
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

/**
 * htp request handler.
 * request: POST ^/ob/plans
 * @param req
 * @param data
 */
static void htp_post_ob_plans(evhtp_request_t *req, void *data)
{
  int ret;
  const char* uuid;
  const char* tmp_const;
  char* tmp;
  json_t* j_data;
  json_t* j_tmp;
  json_t* j_res;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_post_ob_plans.");

  // get uuid
  uuid = req->uri->path->file;
  if(uuid == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // get data
  tmp_const = (char*)evbuffer_pullup(req->buffer_in, evbuffer_get_length(req->buffer_in));
  if(tmp_const == NULL) {
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  // create json
  tmp = strndup(tmp_const, evbuffer_get_length(req->buffer_in));
  slog(LOG_DEBUG, "Requested data. data[%s]", tmp);
  j_data = json_loads(tmp, JSON_DECODE_ANY, NULL);
  sfree(tmp);
  if(j_data == NULL) {
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  // validate plan
  ret = ob_validate_plan(j_data);
  if(ret == false) {
    slog(LOG_DEBUG, "Could not pass the validation.");
    json_decref(j_data);
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  // create plan
  j_tmp = ob_create_plan(j_data);
  json_decref(j_data);
  if(j_tmp == NULL) {
    slog(LOG_INFO, "Could not create ob_plan.");
    http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
    return;
  }

  // create result
  j_res = http_create_default_result(EVHTP_RES_OK);
  json_object_set_new(j_res, "result", j_tmp);

  // response
  http_simple_response_normal(req, j_res);
  json_decref(j_res);

  return;
}

/**
 * htp request handler.
 * request: GET ^/ob/plans/<uuid>
 * @param req
 * @param data
 */
static void htp_get_ob_plans_uuid(evhtp_request_t *req, void *data)
{
  const char* uuid;
  json_t* j_tmp;
  json_t* j_res;
  int ret;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_get_ob_plans_uuid.");

  // get uuid
  uuid = req->uri->path->file;
  if(uuid == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // check existence
  ret = ob_is_plan_exist(uuid);
  if(ret == false) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // get plan info
  j_tmp = ob_get_plan(uuid);
  if(j_tmp == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // create result
  j_res = http_create_default_result(EVHTP_RES_OK);
  json_object_set_new(j_res, "result", j_tmp);

  // response
  http_simple_response_normal(req, j_res);
  json_decref(j_res);

  return;
}

/**
 * htp request handler.
 * request: GET ^/ob/plans/
 * @param req
 * @param data
 */
static void htp_get_ob_plans_all(evhtp_request_t *req, void *data)
{
  json_t* j_tmp;
  json_t* j_res;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_get_ob_plans_all.");

  // get plan info
  j_tmp = ob_get_plans_all();
  if(j_tmp == NULL) {
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

/**
 * htp request handler.
 * request: PUT ^/ob/plans/<uuid>
 * @param req
 * @param data
 */
static void htp_put_ob_plans_uuid(evhtp_request_t *req, void *data)
{
  const char* uuid;
  const char* tmp_const;
  char* tmp;
  json_t* j_data;
  json_t* j_tmp;
  json_t* j_res;
  int ret;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_put_ob_plans_uuid.");

  // get uuid
  uuid = req->uri->path->file;
  if(uuid == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // check existence
  ret = ob_is_plan_exist(uuid);
  if(ret == false) {
    slog(LOG_NOTICE, "Could not find correct ob_plan info. uuid[%s]", uuid);
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // get data
  tmp_const = (char*)evbuffer_pullup(req->buffer_in, evbuffer_get_length(req->buffer_in));
  if(tmp_const == NULL) {
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  // create json
  tmp = strndup(tmp_const, evbuffer_get_length(req->buffer_in));
  slog(LOG_DEBUG, "Requested data. data[%s]", tmp);
  j_data = json_loads(tmp, JSON_DECODE_ANY, NULL);
  sfree(tmp);
  if(j_data == NULL) {
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  // validate requested data
  ret = ob_validate_plan(j_data);
  if(ret == false) {
    slog(LOG_DEBUG, "Could not pass validation.");
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  // update info
  json_object_set_new(j_data, "uuid", json_string(uuid));
  j_tmp = ob_update_plan(j_data);
  json_decref(j_data);
  if(j_tmp == NULL) {
    http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
    return;
  }

  // create result
  j_res = http_create_default_result(EVHTP_RES_OK);
  json_object_set_new(j_res, "result", j_tmp);

  // response
  http_simple_response_normal(req, j_res);
  json_decref(j_res);

  return;
}

/**
 * htp request handler.
 * request: DELETE ^/ob/plans/<uuid>
 * @param req
 * @param data
 */
static void htp_delete_ob_plans_uuid(evhtp_request_t *req, void *data)
{
  const char* uuid;
  const char* tmp_const;
  json_t* j_tmp;
  json_t* j_res;
  int ret;
  int force;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_delete_ob_plans_uuid.");

  // get uuid
  uuid = req->uri->path->file;
  if(uuid == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // check existence
  ret = ob_is_plan_exist(uuid);
  if(ret == false) {
    slog(LOG_NOTICE, "Could not find correct ob_plan info. uuid[%s]", uuid);
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // get parameters
  force = 0;
  tmp_const = evhtp_kv_find(req->uri->query, "force");
  if(tmp_const != NULL) {
    force = atoi(tmp_const);
  }
  slog(LOG_DEBUG, "Check force option. force[%d]", force);

  // check force option
  if(force == 1) {
    ret = ob_clear_campaign_plan(uuid);
    if(ret == false) {
      slog(LOG_ERR, "Could not clear destination info from campaign. dest_uuid[%s]", uuid);
      http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
      return;
    }
  }

  // check deletable
  ret = ob_is_plan_deletable(uuid);
  if(ret == false) {
    slog(LOG_NOTICE, "The given plan info is deletable. uuid[%s]", uuid);
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  // delete info
  j_tmp = ob_delete_plan(uuid);
  if(j_tmp == NULL) {
    http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
    return;
  }

  // create result
  j_res = http_create_default_result(EVHTP_RES_OK);
  json_object_set_new(j_res, "result", j_tmp);

  // response
  http_simple_response_normal(req, j_res);
  json_decref(j_res);

  return;
}

/**
 * htp request handler.
 * request: GET ^/ob/campaignns
 * @param req
 * @param data
 */
static void htp_get_ob_campaigns(evhtp_request_t *req, void *data)
{
  json_t* j_tmp;
  json_t* j_res;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_get_ob_campaigns.");

  // get info
  j_tmp = ob_get_campaigns_all();
  if(j_tmp == NULL) {
    http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
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

/**
 * htp request handler.
 * request: POST ^/ob/campaigns
 * @param req
 * @param data
 */
static void htp_post_ob_campaigns(evhtp_request_t *req, void *data)
{
  const char* uuid;
  const char* tmp_const;
  char* tmp;
  json_t* j_data;
  json_t* j_tmp;
  json_t* j_res;
  int ret;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_post_ob_campaigns.");

  // get uuid
  uuid = req->uri->path->file;
  if(uuid == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // get data
  tmp_const = (char*)evbuffer_pullup(req->buffer_in, evbuffer_get_length(req->buffer_in));
  if(tmp_const == NULL) {
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  // create json
  tmp = strndup(tmp_const, evbuffer_get_length(req->buffer_in));
  slog(LOG_DEBUG, "Requested data. data[%s]", tmp);
  j_data = json_loads(tmp, JSON_DECODE_ANY, NULL);
  sfree(tmp);
  if(j_data == NULL) {
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  ret = ob_validate_campaign(j_data);
  if(ret == false) {
    slog(LOG_NOTICE, "Could not pass the validation.");
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  // create campaigb
  j_tmp = ob_create_campaign(j_data);
  json_decref(j_data);
  if(j_tmp == NULL) {
    slog(LOG_INFO, "Could not create ob_plan.");
    http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
    return;
  }

  // create result
  j_res = http_create_default_result(EVHTP_RES_OK);
  json_object_set_new(j_res, "result", j_tmp);

  // response
  http_simple_response_normal(req, j_res);
  json_decref(j_res);

  return;
}

/**
 * htp request handler.
 * request: GET ^/ob/campaigns/
 * @param req
 * @param data
 */
static void htp_get_ob_campaigns_all(evhtp_request_t *req, void *data)
{
  json_t* j_tmp;
  json_t* j_res;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_get_ob_campaigns_all.");

  // get all campaigns info
  j_tmp = ob_get_campaigns_all();
  if(j_tmp == NULL) {
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

/**
 * htp request handler.
 * request: GET ^/ob/campaigns/<uuid>
 * @param req
 * @param data
 */
static void htp_get_ob_campaigns_uuid(evhtp_request_t *req, void *data)
{
  const char* uuid;
  json_t* j_tmp;
  json_t* j_res;
  int ret;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_get_ob_campaigns_uuid.");

  // get uuid
  uuid = req->uri->path->file;
  if(uuid == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // check existence
  ret = ob_is_exist_campaign(uuid);
  if(ret == false) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // get campaign info
  j_tmp = ob_get_campaign(uuid);
  if(j_tmp == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // create result
  j_res = http_create_default_result(EVHTP_RES_OK);
  json_object_set_new(j_res, "result", j_tmp);

  // response
  http_simple_response_normal(req, j_res);
  json_decref(j_res);

  return;
}

/**
 * htp request handler.
 * request: PUT ^/ob/campaigns/<uuid>
 * @param req
 * @param data
 */
static void htp_put_ob_campaigns_uuid(evhtp_request_t *req, void *data)
{
  const char* uuid;
  const char* tmp_const;
  char* tmp;
  json_t* j_data;
  json_t* j_tmp;
  json_t* j_res;
  int ret;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_put_ob_campaigns_uuid.");

  // get uuid
  uuid = req->uri->path->file;
  if(uuid == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // check existence
  ret = ob_is_exist_campaign(uuid);
  if(ret == false) {
    slog(LOG_NOTICE, "Could not find correct ob_campaign info. uuid[%s]", uuid);
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // get data
  tmp_const = (char*)evbuffer_pullup(req->buffer_in, evbuffer_get_length(req->buffer_in));
  if(tmp_const == NULL) {
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  // create json
  tmp = strndup(tmp_const, evbuffer_get_length(req->buffer_in));
  slog(LOG_DEBUG, "Requested data. data[%s]", tmp);
  j_data = json_loads(tmp, JSON_DECODE_ANY, NULL);
  sfree(tmp);
  if(j_data == NULL) {
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  // update info
  json_object_set_new(j_data, "uuid", json_string(uuid));
  j_tmp = ob_update_campaign(j_data);
  json_decref(j_data);
  if(j_tmp == NULL) {
    http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
    return;
  }

  // create result
  j_res = http_create_default_result(EVHTP_RES_OK);
  json_object_set_new(j_res, "result", j_tmp);

  // response
  http_simple_response_normal(req, j_res);
  json_decref(j_res);

  return;
}

/**
 * htp request handler.
 * request: DELETE ^/ob/campaigns/<uuid>
 * @param req
 * @param data
 */
static void htp_delete_ob_campaigns_uuid(evhtp_request_t *req, void *data)
{
  const char* uuid;
  json_t* j_tmp;
  json_t* j_res;
  int ret;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_delete_ob_campaigns_uuid.");

  // get uuid
  uuid = req->uri->path->file;
  if(uuid == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // check existence
  ret = ob_is_exist_campaign(uuid);
  if(ret == false) {
    slog(LOG_NOTICE, "Could not find correct ob_campaign info. uuid[%s]", uuid);
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // delete info
  j_tmp = ob_delete_campaign(uuid);
  if(j_tmp == NULL) {
    http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
    return;
  }

  // create result
  j_res = http_create_default_result(EVHTP_RES_OK);
  json_object_set_new(j_res, "result", j_tmp);

  // response
  http_simple_response_normal(req, j_res);
  json_decref(j_res);

  return;
}


/**
 * htp request handler.
 * request: GET ^/ob/dlmas
 * @param req
 * @param data
 */
static void htp_get_ob_dlmas(evhtp_request_t *req, void *data)
{
  json_t* j_tmp;
  json_t* j_res;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_get_ob_dlmas.");

  // get info
  j_tmp = ob_get_dlmas_all();
  if(j_tmp == NULL) {
    http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
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

/**
 * htp request handler.
 * request: POST ^/ob/dlmas
 * @param req
 * @param data
 */
static void htp_post_ob_dlmas(evhtp_request_t *req, void *data)
{
  int ret;
  const char* uuid;
  const char* tmp_const;
  char* tmp;
  json_t* j_data;
  json_t* j_tmp;
  json_t* j_res;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_post_ob_dlmas.");

  // get uuid
  uuid = req->uri->path->file;
  if(uuid == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // get data
  tmp_const = (char*)evbuffer_pullup(req->buffer_in, evbuffer_get_length(req->buffer_in));
  if(tmp_const == NULL) {
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  // create json
  tmp = strndup(tmp_const, evbuffer_get_length(req->buffer_in));
  slog(LOG_DEBUG, "Requested data. data[%s]", tmp);
  j_data = json_loads(tmp, JSON_DECODE_ANY, NULL);
  sfree(tmp);
  if(j_data == NULL) {
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  ret = ob_validate_dlma(j_data);
  if(ret == false) {
    slog(LOG_INFO, "Could not pass the validation.");
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  // create plan
  j_tmp = ob_create_dlma(j_data);
  json_decref(j_data);
  if(j_tmp == NULL) {
    slog(LOG_INFO, "Could not create ob_plan.");
    http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
    return;
  }

  // create result
  j_res = http_create_default_result(EVHTP_RES_OK);
  json_object_set_new(j_res, "result", j_tmp);

  // response
  http_simple_response_normal(req, j_res);
  json_decref(j_res);

  return;
}

/**
 * htp request handler.
 * request: GET ^/ob/dlmas/
 * @param req
 * @param data
 */
static void htp_get_ob_dlmas_all(evhtp_request_t *req, void *data)
{
  json_t* j_tmp;
  json_t* j_res;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_get_ob_dlmas_all.");

  // get info
  j_tmp = ob_get_dlmas_all();
  if(j_tmp == NULL) {
    http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
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

/**
 * htp request handler.
 * request: GET ^/ob/dlmas/<uuid>
 * @param req
 * @param data
 */
static void htp_get_ob_dlmas_uuid(evhtp_request_t *req, void *data)
{
  const char* uuid;
  json_t* j_tmp;
  json_t* j_res;
  int ret;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_get_ob_dlmas_uuid.");

  // get uuid
  uuid = req->uri->path->file;
  if(uuid == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // check existence
  ret = ob_is_dlma_exist(uuid);
  if(ret == false) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // get plan info
  j_tmp = ob_get_dlma(uuid);
  if(j_tmp == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // create result
  j_res = http_create_default_result(EVHTP_RES_OK);
  json_object_set_new(j_res, "result", j_tmp);

  // response
  http_simple_response_normal(req, j_res);
  json_decref(j_res);

  return;
}

/**
 * htp request handler.
 * request: PUT ^/ob/dlmas/<uuid>
 * @param req
 * @param data
 */
static void htp_put_ob_dlmas_uuid(evhtp_request_t *req, void *data)
{
  const char* uuid;
  const char* tmp_const;
  char* tmp;
  json_t* j_data;
  json_t* j_tmp;
  json_t* j_res;
  int ret;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_put_ob_dlmas_uuid.");

  // get uuid
  uuid = req->uri->path->file;
  if(uuid == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // check existence
  ret = ob_is_dlma_exist(uuid);
  if(ret == false) {
    slog(LOG_NOTICE, "Could not find correct ob_dlma info. uuid[%s]", uuid);
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // get data
  tmp_const = (char*)evbuffer_pullup(req->buffer_in, evbuffer_get_length(req->buffer_in));
  if(tmp_const == NULL) {
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  // create json
  tmp = strndup(tmp_const, evbuffer_get_length(req->buffer_in));
  slog(LOG_DEBUG, "Requested data. data[%s]", tmp);
  j_data = json_loads(tmp, JSON_DECODE_ANY, NULL);
  sfree(tmp);
  if(j_data == NULL) {
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  // update info
  json_object_set_new(j_data, "uuid", json_string(uuid));
  j_tmp = ob_update_dlma(j_data);
  json_decref(j_data);
  if(j_tmp == NULL) {
    http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
    return;
  }

  // create result
  j_res = http_create_default_result(EVHTP_RES_OK);
  json_object_set_new(j_res, "result", j_tmp);

  // response
  http_simple_response_normal(req, j_res);
  json_decref(j_res);

  return;
}

/**
 * htp request handler.
 * request: DELETE ^/ob/dlmas/<uuid>
 * @param req
 * @param data
 */
static void htp_delete_ob_dlmas_uuid(evhtp_request_t *req, void *data)
{
  const char* uuid;
  const char* tmp_const;
  json_t* j_tmp;
  json_t* j_res;
  int ret;
  int force;
  int delete_dls;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_delete_ob_dlmas_uuid.");

  // get uuid
  uuid = req->uri->path->file;
  if(uuid == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // check existence
  ret = ob_is_dlma_exist(uuid);
  if(ret == false) {
    slog(LOG_NOTICE, "Could not find correct ob_dlma info. uuid[%s]", uuid);
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // get parameters
  force = 0;
  tmp_const = evhtp_kv_find(req->uri->query, "force");
  if(tmp_const != NULL) {
    force = atoi(tmp_const);
  }
  delete_dls = 0;
  tmp_const = evhtp_kv_find(req->uri->query, "delete_dls");
  if(tmp_const != NULL) {
    delete_dls = atoi(tmp_const);
  }
  slog(LOG_DEBUG, "Check force option. force[%d], delete_dls[%d]", force, delete_dls);

  // check force option
  if(force == 1) {
    ret = ob_clear_campaign_dlma(uuid);
    if(ret == false) {
      slog(LOG_ERR, "Could not clear destination info from campaign. dest_uuid[%s]", uuid);
      http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
      return;
    }
  }

  // check delete_dls option
  if(delete_dls == 1) {
    ret = ob_delete_dls_by_dlma_uuid(uuid);
    if(ret == false) {
      slog(LOG_ERR, "Could not delete ob_dls info. dlma_uuid[%s]", uuid);
      http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
      return;
    }
  }

  // check deletable
  ret = ob_is_dlma_deletable(uuid);
  if(ret == false) {
    slog(LOG_NOTICE, "Given dlma info is not deletable. dlma_uuid[%s]", uuid);
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  // delete info
  j_tmp = ob_delete_dlma(uuid);
  if(j_tmp == NULL) {
    http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
    return;
  }

  // create result
  j_res = http_create_default_result(EVHTP_RES_OK);
  json_object_set_new(j_res, "result", j_tmp);

  // response
  http_simple_response_normal(req, j_res);
  json_decref(j_res);

  return;
}

/**
 * htp request handler.
 * request: GET ^/ob/dls
 * @param req
 * @param data
 */
static void htp_get_ob_dls(evhtp_request_t *req, void *data)
{
  json_t* j_tmp;
  json_t* j_res;
  const char* dlma_uuid;
  const char* tmp_const;
  int count;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_get_ob_dls.");

  // get params
  dlma_uuid = evhtp_kv_find(req->uri->query, "dlma_uuid");
  if(dlma_uuid == NULL) {
    slog(LOG_NOTICE, "Could not get correct dlma_uuid.");
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  count = 10000;  /// default count
  tmp_const = evhtp_kv_find(req->uri->query, "count");
  if(tmp_const != NULL) {
    count = atoi(tmp_const);
  }

  // get info
  j_tmp = ob_get_dls_by_dlma_count(dlma_uuid, count);
  if(j_tmp == NULL) {
    http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
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

/**
 * htp request handler.
 * request: POST ^/ob/dls
 * @param req
 * @param data
 */
static void htp_post_ob_dls(evhtp_request_t *req, void *data)
{
  const char* uuid;
  const char* tmp_const;
  char* tmp;
  json_t* j_data;
  json_t* j_tmp;
  json_t* j_res;
  int ret;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_post_ob_dls.");

  // get uuid
  uuid = req->uri->path->file;
  if(uuid == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // get data
  tmp_const = (char*)evbuffer_pullup(req->buffer_in, evbuffer_get_length(req->buffer_in));
  if(tmp_const == NULL) {
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  // create json
  tmp = strndup(tmp_const, evbuffer_get_length(req->buffer_in));
  slog(LOG_DEBUG, "Requested data. data[%s]", tmp);
  j_data = json_loads(tmp, JSON_DECODE_ANY, NULL);
  sfree(tmp);
  if(j_data == NULL) {
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  // validate data
  ret = ob_validate_dl(j_data);
  if(ret == false) {
    json_decref(j_data);
    slog(LOG_ERR, "Could not pass the ob_dl validate.");
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  // create plan
  j_tmp = ob_create_dl(j_data);
  json_decref(j_data);
  if(j_tmp == NULL) {
    slog(LOG_INFO, "Could not create ob_plan.");
    http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
    return;
  }

  // create result
  j_res = http_create_default_result(EVHTP_RES_OK);
  json_object_set_new(j_res, "result", j_tmp);

  // response
  http_simple_response_normal(req, j_res);
  json_decref(j_res);

  return;
}

/**
 * htp request handler.
 * request: GET ^/ob/dls/
 * @param req
 * @param data
 */
static void htp_get_ob_dls_all(evhtp_request_t *req, void *data)
{
  json_t* j_tmp;
  json_t* j_res;
  const char* dlma_uuid;
  const char* tmp_const;
  int count;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_get_ob_dls_all.");

  // get params
  dlma_uuid = evhtp_kv_find(req->uri->query, "dlma_uuid");
  if(dlma_uuid == NULL) {
    slog(LOG_NOTICE, "Could not get correct dlma_uuid.");
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  count = 10000;  // default count
  tmp_const = evhtp_kv_find(req->uri->query, "count");
  if(tmp_const != NULL) {
    count = atoi(tmp_const);
  }

  // get info
  j_tmp = ob_get_dls_by_dlma_count(dlma_uuid, count);
  if(j_tmp == NULL) {
    http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
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

/**
 * htp request handler.
 * request: GET ^/ob/dls/<uuid>
 * @param req
 * @param data
 */
static void htp_get_ob_dls_uuid(evhtp_request_t *req, void *data)
{
  const char* uuid;
  json_t* j_tmp;
  json_t* j_res;
  int ret;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_get_ob_dls_uuid.");

  // get uuid
  uuid = req->uri->path->file;
  if(uuid == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // check existence
  ret = ob_is_dl_exsit(uuid);
  if(ret == false) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // get plan info
  j_tmp = ob_get_dl(uuid);
  if(j_tmp == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // create result
  j_res = http_create_default_result(EVHTP_RES_OK);
  json_object_set_new(j_res, "result", j_tmp);

  // response
  http_simple_response_normal(req, j_res);
  json_decref(j_res);

  return;
}

/**
 * htp request handler.
 * request: PUT ^/ob/dls/<uuid>
 * @param req
 * @param data
 */
static void htp_put_ob_dls_uuid(evhtp_request_t *req, void *data)
{
  const char* uuid;
  const char* tmp_const;
  char* tmp;
  json_t* j_data;
  json_t* j_tmp;
  json_t* j_res;
  int ret;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_put_ob_dls_uuid.");

  // get uuid
  uuid = req->uri->path->file;
  if(uuid == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // check existence
  ret = ob_is_dl_exsit(uuid);
  if(ret == false) {
    slog(LOG_NOTICE, "Could not find correct ob_campaign info. uuid[%s]", uuid);
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // get data
  tmp_const = (char*)evbuffer_pullup(req->buffer_in, evbuffer_get_length(req->buffer_in));
  if(tmp_const == NULL) {
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  // create json
  tmp = strndup(tmp_const, evbuffer_get_length(req->buffer_in));
  slog(LOG_DEBUG, "Requested data. data[%s]", tmp);
  j_data = json_loads(tmp, JSON_DECODE_ANY, NULL);
  sfree(tmp);
  if(j_data == NULL) {
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  // validate data
  ret = ob_validate_dl(j_data);
  if(ret == false) {
    json_decref(j_data);
    slog(LOG_ERR, "Could not pass the ob_dl validate.");
    http_simple_response_error(req, EVHTP_RES_BADREQ, 0, NULL);
    return;
  }

  // update info
  json_object_set_new(j_data, "uuid", json_string(uuid));
  j_tmp = ob_update_dl(j_data);
  json_decref(j_data);
  if(j_tmp == NULL) {
    http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
    return;
  }

  // create result
  j_res = http_create_default_result(EVHTP_RES_OK);
  json_object_set_new(j_res, "result", j_tmp);

  // response
  http_simple_response_normal(req, j_res);
  json_decref(j_res);

  return;
}

/**
 * htp request handler.
 * request: DELETE ^/ob/dls/<uuid>
 * @param req
 * @param data
 */
static void htp_delete_ob_dls_uuid(evhtp_request_t *req, void *data)
{
  const char* uuid;
  json_t* j_tmp;
  json_t* j_res;
  int ret;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_delete_ob_dls_uuid.");

  // get uuid
  uuid = req->uri->path->file;
  if(uuid == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // check existence
  ret = ob_is_dl_exsit(uuid);
  if(ret == false) {
    slog(LOG_NOTICE, "Could not find correct ob_dl info. uuid[%s]", uuid);
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // delete info
  j_tmp = ob_delete_dl(uuid);
  if(j_tmp == NULL) {
    http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
    return;
  }

  // create result
  j_res = http_create_default_result(EVHTP_RES_OK);
  json_object_set_new(j_res, "result", j_tmp);

  // response
  http_simple_response_normal(req, j_res);
  json_decref(j_res);

  return;
}

/**
 * http request handler.
 * request : ^/ob/dialings
 * @param req
 * @param data
 */
void ob_cb_htp_ob_dialings(evhtp_request_t *req, void *data)
{
  int method;
  int ret;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired cb_htp_ob_dialings.");

  // check authorization
  ret = http_is_request_has_permission(req, EN_HTTP_PERM_ADMIN);
  if(ret == false) {
    http_simple_response_error(req, EVHTP_RES_FORBIDDEN, 0, NULL);
    return;
  }

  // method check
  method = evhtp_request_get_method(req);
  if((method != htp_method_GET) ) {
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
    return;
  }

  if(method == htp_method_GET) {
    htp_get_ob_dialings(req, data);
    return;
  }
  else {
    // should not reach to here.
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
  }

  // should not reach to here.
  http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);

  return;
}

/**
 * http request handler.
 * request : ^/ob/dialings/
 * @param req
 * @param data
 */
void ob_cb_htp_ob_dialings_all(evhtp_request_t *req, void *data)
{
  int method;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired cb_htp_ob_dialings.");

  // method check
  method = evhtp_request_get_method(req);
  if((method != htp_method_GET) ) {
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
    return;
  }

  if(method == htp_method_GET) {
    htp_get_ob_dialings_all(req, data);
    return;
  }
  else {
    // should not reach to here.
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
  }

  // should not reach to here.
  http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);

  return;
}


/**
 * htp request handler.
 * request: GET ^/ob/dialings
 * @param req
 * @param data
 */
static void htp_get_ob_dialings(evhtp_request_t *req, void *data)
{
  json_t* j_tmp;
  json_t* j_res;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_get_ob_dialings.");

  // get info
  j_tmp = ob_get_dialings_all();
  if(j_tmp == NULL) {
    http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
    return;
  }

  // create result
  j_res = http_create_default_result(EVHTP_RES_OK);
  if(j_res == NULL) {
    http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
    return;
  }
  json_object_set_new(j_res, "result", json_object());
  json_object_set_new(json_object_get(j_res, "result"), "list", j_tmp);

  // response
  http_simple_response_normal(req, j_res);
  json_decref(j_res);

  return;
}

/**
 * htp request handler.
 * request: GET ^/ob/dialings/
 * @param req
 * @param data
 */
static void htp_get_ob_dialings_all(evhtp_request_t *req, void *data)
{
  json_t* j_tmp;
  json_t* j_res;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_get_ob_dialings_all.");

  // get info
  j_tmp = ob_get_dialings_all();
  if(j_tmp == NULL) {
    http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
    return;
  }

  // create result
  j_res = http_create_default_result(EVHTP_RES_OK);
  if(j_res == NULL) {
    http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
    return;
  }
  json_object_set_new(j_res, "result", json_object());
  json_object_set_new(json_object_get(j_res, "result"), "list", j_tmp);

  // response
  http_simple_response_normal(req, j_res);
  json_decref(j_res);

  return;
}

/**
 * http request handler.
 * request : ^/ob/dialings/<uuid>
 * @param req
 * @param data
 */
void ob_cb_htp_ob_dialings_detail(evhtp_request_t *req, void *data)
{
  int method;
  const char* uuid;
  int ret;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired cb_htp_ob_dialings_uuid.");

  // check authorization
  ret = http_is_request_has_permission(req, EN_HTTP_PERM_ADMIN);
  if(ret == false) {
    http_simple_response_error(req, EVHTP_RES_FORBIDDEN, 0, NULL);
    return;
  }

  // method check
  method = evhtp_request_get_method(req);
  if((method != htp_method_GET) && (method != htp_method_DELETE)) {
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
    return;
  }

  // get uuid
  uuid = req->uri->path->match_start;
  if(uuid == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  if(method == htp_method_GET) {
    htp_get_ob_dialings_uuid(req, data);
    return;
  }
  else if(method == htp_method_DELETE) {
    htp_delete_ob_dialings_uuid(req, data);
    return;
  }
  else {
    // should not reach to here.
    http_simple_response_error(req, EVHTP_RES_METHNALLOWED, 0, NULL);
    return;
  }

  // should not reach to here.
  http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);

  return;
}

/**
 * htp request handler.
 * request: GET /ob/dialings/<uuid>
 * @param req
 * @param data
 */
static void htp_get_ob_dialings_uuid(evhtp_request_t *req, void *data)
{
  const char* uuid;
  json_t* j_tmp;
  json_t* j_res;
  int ret;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }

  // get uuid
  uuid = req->uri->path->file;
  if(uuid == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // check existence
  ret = ob_is_exist_dialing(uuid);
  if(ret == false) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // get destination info
  j_tmp = ob_get_dialing(uuid);
  if(j_tmp == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // create result
  j_res = http_create_default_result(EVHTP_RES_OK);
  json_object_set_new(j_res, "result", j_tmp);

  // response
  http_simple_response_normal(req, j_res);
  json_decref(j_res);

  return;
}

/**
 * htp request handler.
 * request: DELETE ^/ob/dialings/<uuid>
 * @param req
 * @param data
 */
static void htp_delete_ob_dialings_uuid(evhtp_request_t *req, void *data)
{
  const char* uuid;
  json_t* j_res;
  int ret;

  if(req == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return;
  }
  slog(LOG_DEBUG, "Fired htp_delete_ob_dialings_uuid.");

  // get uuid
  uuid = req->uri->path->file;
  if(uuid == NULL) {
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // check existence
  ret = ob_is_exist_dialing(uuid);
  if(ret == false) {
    slog(LOG_NOTICE, "Could not find correct ob_dlma info. uuid[%s]", uuid);
    http_simple_response_error(req, EVHTP_RES_NOTFOUND, 0, NULL);
    return;
  }

  // send hangup request of ob_dialing
  ret = ob_send_dialing_hangup_request(uuid);
  if(ret == false) {
    http_simple_response_error(req, EVHTP_RES_SERVERR, 0, NULL);
    return;
  }

  // create result
  j_res = http_create_default_result(EVHTP_RES_OK);

  // response
  http_simple_response_normal(req, j_res);
  json_decref(j_res);

  return;
}
