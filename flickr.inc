<?php
// $Id: flickr.inc,v 1.17 2007/04/13 17:21:08 drewish Exp $

define('FLICKR_REST_ENDPOINT', 'http://api.flickr.com/services/rest/');

/**
 * Return a list of all possible photo sizes with the right
 * description and label
 *
 * @return
 *	A list of all photo sizes
 */
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
 *
 * @return
 *   an array with the the result of the request
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
    if ($array = unserialize($result->data)) {
      // save a cacheable result for future use
      if ($cacheable) {
        cache_set($arghash, 'cache', $result->data, time() + 3600);
      }
      return $array;
    }
  }

  return FALSE;
}

/**
 * This function will try to create a html image tag referencing the Flickr
 * photo with the desired size if that size is available for this photo.
 *
 * @param $photo
 *   the photo variable
 * @param $size
 *   the desired image size
 *
 * @return
 *   a html image tag referencing the image of the desired
 *   size if it is available.
 */
function flickr_img($photo, $size = NULL) {
  $sizes = flickr_photo_sizes();
  if (!isset($size)) {
    $size = '-';
  }
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

/**
 * Create the url to $photo with size $size using the correct image farm
 * from the $photo variable
 *
 * @param $photo
 *   photo to which the url should point
 * @param $size
 *   size of the photo
 * @param $format
 *   format of the photo
 *
 * @return
 *   url for $photo with the correct size and format
 */
function flickr_photo_img($photo, $size = NULL, $format = NULL) {
  // early images don't have a farm setting so default to 1.
  $farm = isset($photo['farm']) ? $photo['farm'] : 1;
  $server = $photo['server'];
  // photoset's use primary instead of id to specify the image.
  $id = isset($photo['primary']) ? $photo['primary'] : $photo['id'];
  $secret = $photo['secret'];

  return "http://farm{$farm}.static.flickr.com/{$server}/{$id}_{$secret}" .($size ? "_$size." : '.') . ($size == 'o' ? $format : 'jpg');
}

/**
 * @param $owner
 *   owner of the photo
 * @param $id
 *   id of the photo to reference in the url
 *
 * @return
 *   url for the flickr photo page showing photo with $id of $owner
 */
function flickr_photo_page_url($owner, $id = NULL) {
  $owner = is_array($owner) ? $owner['nsid']: $owner;
  return 'http://flickr.com/photos/'. $owner . ($id ? '/'. $id : '');
}

/**
 * @param $owner
 *   owner of the photoset
 * @param $id
 *   id of the photoset to which the url must lead
 *
 * @return
 *   url for the photoset page of photoset $id of owner $owner
 */
function flickr_photoset_page_url($owner, $id = NULL) {
  $owner = is_array($owner) ? $owner['nsid']: $owner;
  return "http://flickr.com/photos/$owner/sets/$id";
}

/**
 * @param $photo_id
 *   id of the photo to get info about
 *
 * @return
 *   response from the flickr method flickr.photos.getInfo
 *   (http://www.flickr.com/services/api/flickr.photos.getInfo.html)
 */
function flickr_photo_get_info($photo_id) {
  return flickr_request(
    'flickr.photos.getInfo',
    array('photo_id' => $photo_id)
  );
}

/**
 * @param $photo_id
 *   id of the photo to get the available sizes of
 *
 * @return
 *   response from the flickr method flickr.photos.getSizes
 *   (http://www.flickr.com/services/api/flickr.photos.getSizes.html)
 */
function flickr_photo_get_sizes($photo_id) {
  return flickr_request(
    'flickr.photos.getSizes',
    array('photo_id' => $photo_id)
  );
}

/**
 * @param $photoset_id
 *   id of the photoset to get information about
 *
 * @return
 *   response from the flickr method flickr.photosets.getInfo
 *   (http://www.flickr.com/services/api/flickr.photosets.getInfo.html)
 */
function flickr_photoset_get_info($photoset_id) {
  return flickr_request(
    'flickr.photosets.getInfo',
    array('photoset_id' => $photoset_id)
  );
}

/**
 * @param $nsid
 *   nsid of the user whose photoset list you want
 *
 * @return
 *   response from the flickr method flickr.photosets.getList
 *   (http://www.flickr.com/services/api/flickr.photosets.getList.html)
 */
function flickr_photoset_get_list($nsid) {
  return flickr_request('flickr.photosets.getList', array('user_id' => $nsid));
}

/**
 * Tries to match an 'identifier' onto a flickr nsid
 *
 * This function will first see whether $identifier is allready
 * a nsid (format check only, no api call).  If it is not and the
 * identifier has the format of an email, an api call will be made to
 * check whether there is an nsid for that email.  If this is not the
 * case, the $identifier is treated as a username and an api call is
 * made to find the nsid for that username.
 *
 * If none of these succees, the result will be false
 *
 * @param $identifier
 *   identifier to find an nsid for
 *
 * @return
 *   valid nsid or false if none can be found
 */
function flickr_user_find_by_identifier($identifier) {
  if (preg_match('/^\d+@N\d+$/', $identifier)) {
    //identifier is an NSID
    return $identifier;
  }
  if (valid_email_address($identifier) && !flickr_error(($response = flickr_user_find_by_email($identifier)))) {
    return $response['user']['nsid'];
  }
  if (!flickr_error($response = flickr_user_find_by_username($identifier))) {
    return $response['user']['nsid'];
  }

  return FALSE;
}

/**
 * Lookup an nsid for a username.
 *
 * @param $username
 *   username to look for
 *
 * @return
 *   response from the flickr method flickr.people.findByUsername
 *   (http://www.flickr.com/services/api/flickr.people.findByUsername.html)
 */
function flickr_user_find_by_username($username) {
  return flickr_request(
    'flickr.people.findByUsername',
    array('username' => $username)
  );
}

/**
 * Lookup an nsid for an email address
 *
 * @param $email
 *   email to look for
 *
 * @return
 *   response from the flickr method flickr.people.findByEmail
 *   (http://www.flickr.com/services/api/flickr.people.findByEmail.html)
 */
function flickr_user_find_by_email($email) {
  return $response = flickr_request(
    'flickr.people.findByEmail',
    array('find_email' => $email)
  );
}

/**
 * @param $nsid
 *   nsid of the user whose tags will be returned
 * @param $count
 *   number of tags to return
 *
 * @return
 *   response from the flickr method flickr.tags.getListUserPopular
 *   (http://www.flickr.com/services/api/flickr.tags.getListUserPopular.html)
 */
function flickr_tags_get_list_user_popular($nsid, $count=NULL) {
  $args = array('user_id'=>$nsid);
  if ($count != NULL) {
    $args['count'] = $count;
  }
  return flickr_request('flickr.tags.getListUserPopular', $args);
}

/**
 * @param $nsid
 *   nsid of the user whose photoset tags will be returned
 *
 * @return
 *   response from the flickr method flickr.tags.getListUser
 *   (http://www.flickr.com/services/api/flickr.tags.getListUser.html)
 */
function flickr_tags_get_list_user($nsid){
  return $response = flickr_request(
    'flickr.tags.getListUser',
    array('user_id' => $nsid)
  );
}

/**
 * Check if the response from the Flickr api call was an error
 *
 * @param $response
 *   response to check
 *
 * @return
 *    true if the response is an error message
 */
function flickr_error($response) {
  if (array_key_exists('stat', $response) && $response['stat'] == 'ok') {
    return FALSE;
  }
  else {
    return TRUE;
  }
}

/**
 * Display an error message and write an error to watchdog.
 * Only call this method if flickr_error returns TRUE.
 *
 * @param $response
 *   response to display
 */
function flickr_set_error($response) {
  $err_message = t('Flickr error @error_id: %flickr_error', array('@error_id'=>$response['code'],
        '%flickr_error'=> $response['message']));
  drupal_set_message($err_message, 'error');
  watchdog('flickr', $err_message, WATCHDOG_WARNING);
}

