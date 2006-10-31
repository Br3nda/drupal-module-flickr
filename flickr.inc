<?php
// $Id: flickr.inc,v 1.4 2006/10/31 00:30:16 drewish Exp $

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
    if ($cache = cache_get($arghash, 'cache')) {
      return unserialize($cache->data);
    }
  }

  // connect and fetch a value
  if ($result = file_get_contents($url)) {
    // save a cacheable result for future use
    if ($cacheable) {
      cache_set($arghash, 'cache', $result);
    }
    return unserialize($result);
  }

  return FALSE;
}
