<?php
// $Id: flickr.inc,v 1.3.2.2 2006/11/03 02:37:26 drewish Exp $

define('FLICKR_REST_ENDPOINT', 'http://api.flickr.com/services/rest/');

/**
 * Submit a request to Flickr.
 *
 * @param $method
 *   string method name
 * @param $args
 *   associative array of arguments names and values
 * @param $cacheable
 *   boolean indicating if it's safe cache the results of this request
 */
function flickr_request($method, $args, $cacheable = TRUE) {
  $args['api_key'] = trim(variable_get('flickr_api_key', ''));
  $args['method'] = $method;
  $args['format'] = 'php_serial';
  ksort($args);

  // build an argument hash that we'll use for the cache id and api signing
  $arghash = '';
  foreach($args as $k => $v){
    $arghash .= $k . $v;
  }
  // if we've got a secret sign the arguments
  if ($secret = trim(variable_get('flickr_api_secret', ''))) {
    $args['api_sig'] = md5($secret . $arghash);
  }

  // build the url
  foreach ($args as $k => $v){
    $encoded_params[] = urlencode($k).'='.urlencode($v);
  }
  $url = FLICKR_REST_ENDPOINT .'?'. implode('&', $encoded_params);

  // if it's a cachable request, try to load a cached value
  if ($cacheable) {
    if ($cache = cache_get($arghash)) {
      return unserialize($cache->data);
    }
  }

  // connect and fetch a value
  $result = drupal_http_request($url);
  if ($result->code == 200) {
    // make sure it unserializes
    if ($object = unserialize($result->data)) {
      // save a cacheable result for future use
      if ($cacheable) {
        cache_set($arghash, $result->data);
      }
      return $object;
    }
  }

  return FALSE;
}

function flickr_photo_img($server, $id, $secret, $size = NULL, $format = NULL) {
  return "http://static.flickr.com/{$server}/{$id}_{$secret}" .($size ? "_$size." : '.') . ($size == 'o' ? $format : 'jpg');
}

function flickr_photo_page_url($owner, $id = NULL) {
  return 'http://flickr.com/photos/'. $owner . ($id ? '/'. $id : '');
}

function flickr_photoset_page_url($owner, $id = NULL) {
  return "http://flickr.com/photos/$owner/sets/$id";
}
