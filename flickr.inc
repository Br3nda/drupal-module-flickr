<?php
// $Id: flickr.inc,v 1.1 2006/09/18 01:41:18 drewish Exp $

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
  ksort($arguments);

  if ($secret = variable_get('flickr_api_secret', '')) {
    $arguments['api_sig'] = _flickr_sign_args($secret, $arguments);
  }

  if (!$cacheable) {
    // connect and fetch it.
    $result = xmlrpc(FLICKR_XMLRPC_ENDPOINT, $method, $arguments);
    if ($result) {
      return _flickr_object_from_xml($result);
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
      return _flickr_object_from_xml($cached->data);
    }
    else {
      // connect and fetch a value
      $result = xmlrpc(FLICKR_XMLRPC_ENDPOINT, $method, $arguments);
      // if we got a value save it.
      if ($result) {
        cache_set($cacheid, 'cache', $result);
        return _flickr_object_from_xml($result);
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


/**
 * Create an object from XML.
 * http://www.php.net/manual/tw/function.xml-parse-into-struct.php#66487
 */
function _flickr_object_from_xml($xml) {
  $parser = xml_parser_create();
  xml_parser_set_option($parser, XML_OPTION_CASE_FOLDING, 0);
  xml_parser_set_option($parser, XML_OPTION_SKIP_WHITE, 1);
  xml_parse_into_struct($parser, $xml, $tags);
  xml_parser_free($parser);

  $elements = array();  // the currently filling [child] XmlElement array
  $stack = array();
  foreach ($tags as $tag) {
    $index = count($elements);
    if ($tag['type'] == "complete" || $tag['type'] == "open") {
      $elements[$index] = new stdClass;
      $elements[$index]->name = $tag['tag'];
      $elements[$index]->attributes = $tag['attributes'];
      $elements[$index]->content = $tag['value'];
      if ($tag['type'] == "open") {  // push
        $elements[$index]->children = array();
        $stack[count($stack)] = &$elements;
        $elements = &$elements[$index]->children;
      }
    }
    if ($tag['type'] == "close") {  // pop
      $elements = &$stack[count($stack) - 1];
      unset($stack[count($stack) - 1]);
    }
  }
  return $elements[0];  // the single top-level element
}
