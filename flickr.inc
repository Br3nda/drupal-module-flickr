<?php
// $Id: flickr.inc,v 1.11 2007/01/01 22:09:39 drewish Exp $

define('FLICKR_REST_ENDPOINT', 'http://api.flickr.com/services/rest/');

function flickr_photo_sizes() {
  return array(
    's' => array(
      'label' => 'Square',
      'description' => t('75x75 pixel square'),
    ),
    't' => array(
      'label' => 'Thumbnail',
      'description' => t('100 pixels on longest side'),
    ),
    'm' => array(
      'label' => 'Small',
      'description' => t('240 pixels on longest side'),
    ),
    '-' => array(
      'label' => 'Medium',
      'description' => t('500 pixels on longest side'),
    ),
    'b' => array(
      'label' => 'Large',
      'description' => t('1024 pixels on longest side'),
    ),
    'o' => array(
      'label' => 'Original',
      'description' => t('Original image')
    ),
  );
};

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
  $result = drupal_http_request($url);
  if ($result->code == 200) {
    // make sure it unserializes
    if ($object = unserialize($result->data)) {
      // save a cacheable result for future use
      if ($cacheable) {
        cache_set($arghash, 'cache', $result->data, time() + 3600);
      }
      return $object;
    }
  }

  return FALSE;
}

function flickr_img($photo, $size = '-') {
  $sizes = flickr_photo_sizes();
  if (!isset($sizes[$size])) {
    return;
  }
  
  $attributes = array();
  // photoset's use primary instead of id to specify the image.
  if (isset($photo['primary'])) {
    $id = $photo['primary'];
    $attributes['class'] = 'flickr-photoset-img';
  } 
  else {
    $id = $photo['id'];
    $attributes['class'] = 'flickr-photo-img';
  }
  
  if ($size == 's') {
    $attributes['height'] = $attributes['width'] = 75;
    $img_url = flickr_photo_img($photo, $size);
  }
  else {
    $response = flickr_photo_get_sizes($id);
    if ($response) {
      foreach ($response['sizes']['size'] as $imagesize) {
        if ($imagesize['label'] == $sizes[$size]['label']) {
          break;
        }
      }
      if (isset($imagesize)) {
        $img_url = $imagesize['source'];
        $attributes['height'] = $imagesize['height'];
        $attributes['width'] = $imagesize['width'];
      }
    }
    else {
      $img_url = flickr_photo_img($photo, $size, $format);
    }
  }
  $title = is_array($p['title']) ? $p['title']['_content'] : $p['title'];
  return theme('image', $img_url, $title, $title, $attributes, FALSE);
}


function flickr_photo_img($photo, $size = NULL, $format = NULL) {
  // early images don't have a farm setting so default to 1.
  $farm = isset($photo['farm']) ? $photo['farm'] : 1;
  $server = $photo['server'];
  // photoset's use primary instead of id to specify the image.
  $id = isset($photo['primary']) ? $photo['primary'] : $photo['id'];
  $secret = $photo['secret'];
  
  return "http://farm{$farm}.static.flickr.com/{$server}/{$id}_{$secret}" .($size ? "_$size." : '.') . ($size == 'o' ? $format : 'jpg');
}

function flickr_photo_page_url($owner, $id = NULL) {
  $owner = is_array($owner) ? $owner['nsid']: $owner;
  return 'http://flickr.com/photos/'. $owner . ($id ? '/'. $id : '');
}

function flickr_photoset_page_url($owner, $id = NULL) {
  $owner = is_array($owner) ? $owner['nsid']: $owner;
  return "http://flickr.com/photos/$owner/sets/$id";
}

function flickr_photo_get_info($photo_id) {
  return flickr_request(
    'flickr.photos.getInfo',
    array('photo_id' => $photo_id)
  );
}

function flickr_photo_get_sizes($photo_id) {
  return flickr_request(
    'flickr.photos.getSizes',
    array('photo_id' => $photo_id)
  );
}

function flickr_photoset_get_info($photoset_id) {
  return flickr_request(
    'flickr.photosets.getInfo',
    array('photoset_id' => $photoset_id)
  );
}

function flickr_user_find_by_username($username) {
  return flickr_request(
    'flickr.people.findByUsername',
    array('username' => $username)
  );
}
