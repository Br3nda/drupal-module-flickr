<?php
// $Id: flickr.inc,v 1.2 2006/10/30 22:31:45 drewish Exp $

define('FLICKR_XMLRPC_ENDPOINT', 'http://www.flickr.com/services/xmlrpc/');

/**
 * Submit a request to Flickr.
 *
 * @param $method
 *   string method name
 * @param $arguments
 *   associative array of arguments names and values
 * @param $cacheable
 *   boolean indicating if it's safe cache the results of this request
 */
function flickr_request($method, $arguments, $cacheable = TRUE) {
  $arguments['api_key'] = variable_get('flickr_api_key', '');
  $arguments['method'] = $method;
  $arguments['format'] = 'php_serial';
  ksort($arguments);

  if ($secret = variable_get('flickr_api_secret', '')) {
    $arguments['api_sig'] = _flickr_sign_args($secret, $arguments);
  }

  foreach ($arguments as $k => $v){
    $encoded_params[] = urlencode($k).'='.urlencode($v);
  }
  $url = "http://api.flickr.com/services/rest/?". implode('&', $encoded_params);

  if (!$cacheable) {
    // connect and fetch it.
    $rsp = file_get_contents($url);
    if ($rsp) {
      return unserialize($result);
    }
  }
  else {
    // build the cacheid
    $cacheid = $method;
    foreach ((array)$arguments as $key => $val) {
      $cacheid .= "$key=$val";
    }

    // try to load a cached value
    $cached = cache_get($cacheid, 'cache');
    if ($cached != 0) {
      return unserialize($cached->data);
    }
    else {
      // connect and fetch a value
      $result = file_get_contents($url);
      // if we got a value save it.
      if ($result) {
        cache_set($cacheid, 'cache', $result);
        return unserialize($result);
      }
    }
  }

  drupal_set_message(xmlrpc_error_msg());
  return FALSE;
}

function _flickr_sign_args($secret, $args) {
  ksort($args);
  $a = '';
  foreach($args as $k => $v){
    $a .= $k . $v;
  }
  return md5($secret . $a);
}
