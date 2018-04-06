<?php

/**
 * @file
 * Drush hooks.
 * @author Diego Pino Navarro
 */

/**
 * The Ace Code Highlight JS library.
 */
define('ACEJS_DOWNLOAD_URI', 'https://github.com/ajaxorg/ace-builds/archive/v1.3.3.zip');


/**
 * Implements hook_drush_command().
 */
function islandora_multi_importer_drush_command() {
  $items = array();

  $items['islandora-multi-importer-zeromqboot'] = array(
    'aliases' => array('imzmqboot'),
    'description' => 'Spawn an Islandora Multi Importer ZeroMQ server/workers ensamble',
    'drupal dependencies' => array('islandora_multi_importer'),
    'callback' => 'drush_islandora_multi_importer_zeromqboot',
    'examples' => array(
      'drush -v --user=admin islandora_multi_importer_zeromqboot',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
  );
  
  $items['islandora-multi-importer-acejs'] = array(
    'callback' => 'drush_islandora_multi_importer_acejs',
    'description' => dt('Download and install the Ace Code Hightligher JS Library.'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
    'arguments' => array(
      'path' => dt('Optional. A path where to install the Ace Code Hightligher JS. If omitted Drush will use the default location.'),
    ),
    'aliases' => array('imiacejs'),
  );
  
  return $items;
}

/**
 * Implements hook_drush_help().
 */
function islandora_multi_importer_drush_help($section) {
  switch ($section) {
    case 'drush:islandora-multi-importer-acejs':
      return dt('Download and install the Ace Code Hightligher JS Library, default location is sites/all/libraries.');
      break;
    case 'drush:islandora-multi-importer-zeromqboot':  
      return dt('Spawns a zeroMQ server and workers. Experimental, do not use on production.');
      break;
  }
}

/**
 * Builds a preprocessor, and passes it off to a preprocessor handler.
 */
function drush_islandora_multi_importer_zeromqboot() {
  // This really requires knowing what you are getting into.
  // Highly experimental. This is a islandora Batch replacement
  // Based on multiple workers and socket communication using
  // zeroMQ. DON'T USE ON PRODUCTION NEVER EVER!
  module_load_include('inc', 'islandora_multi_importer', 'includes/zeroMQ');
  module_load_include('inc', 'islandora_multi_importer', 'includes/zmsg');
  drupal_static_reset('islandora_get_tuque_connection');
  $connection = islandora_get_tuque_connection();
  $boot = islandora_multi_importer_zeroMQ_boot($connection);
}

/**
 * Command to download the The Ace JS library.
 */
function drush_islandora_multi_importer_acejs() {
  $args = func_get_args();
  if (!empty($args[0])) {
    $path = $args[0];
  }
  else {
    $path = _drush_core_directory("@site:sites/all/libraries");
  }
  drush_log($path);
  drush_log(ACEJS_DOWNLOAD_URI);
  // Error if destination does not exist.
  // Lets avoid playing sysadmins with drush.
  if (!is_dir($path) || !is_writable($path) ) {
    drush_log(dt('Drush was unable to install the Ace JS Library to @path', array('@path' => $path)), 'error');
    return FALSE;
  }
  else {
  // Download the library.
    $filepath = drush_download_file(ACEJS_DOWNLOAD_URI, $path);
    drush_log($filepath);
    if ($filepath = drush_download_file(ACEJS_DOWNLOAD_URI, $path)) {
      // Decompress the zip archive.
      $targetdir = $filepath ."/ace";
      drush_tarball_extract($filepath, $targetdir);
    }
  }

  if (is_dir($targetdir)) {
    drush_log(dt('Ace JS Library has been installed in @path', array('@path' => $path)), 'success');
  }
  else {
    drush_log(dt('Drush was unable to install the Ace JS Library to @path', array('@path' => $path)), 'error');
  }
}
