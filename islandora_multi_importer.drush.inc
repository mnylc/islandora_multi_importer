<?php

/**
 * @file
 * Drush hooks.
 * @author Diego Pino Navarro
 */

/**
 * The Ace Code Highlight JS library.
 */
define('ACEJS_DOWNLOAD_URI', 'https://github.com/ajaxorg/ace-builds/archive/');
define('ACEJS_RELEASE_VERSION', '1.3.3');


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
  
  $items['islandora-multi-importer-acejs-plugin'] = array(
    'callback' => 'drush_islandora_multi_importer_acejs_plugin',
    'description' => dt('Download and install the Ace Code Hightligher JS Library.'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
    'arguments' => array(
      'path' => dt('Optional. A path where to install the Ace Code Hightligher JS. If omitted Drush will use the default location.'),
    ),
    'aliases' => array('imi-aceplugin'),
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
 * Implements drush_MODULE_pre_pm_enable().
 */
function drush_islandora_multi_importer_pre_pm_enable() {
   $modules = drush_get_context('PM_ENABLE_MODULES');
   if (in_array('islandora_multi_importer', $modules) && !drush_get_option('skip')) {
     drush_islandora_multi_importer_acejs_plugin();
   }
}

/**
* Command to download the The Ace JS library.
*/
function drush_islandora_multi_importer_acejs_plugin() {
  $args = func_get_args();
  if (!empty($args[0])) {
    $path = $args[0];
  }
  else {
    $path = _drush_core_directory("@site:sites/all/libraries");
  }

  // Create the path if it does not exist.
  if (!is_dir($path)) {
    drush_op('mkdir', $path);
    drush_log(dt('Directory @path was created', array('@path' => $path)), 'notice');
  }

  // Download ace plugin only if path is writable.
  if (is_writable($path)) {
    // Set the directory to the download location.
    $olddir = getcwd();
    chdir($path);

    // Download the zip archive
    if ($filepath = drush_download_file(ACEJS_DOWNLOAD_URI."v".ACEJS_RELEASE_VERSION.".zip")) {
      $filename = basename($filepath);
      $dirname = "ace-builds-".ACEJS_RELEASE_VERSION;
      // Remove any existing ace js plugin directory.
      if (is_dir('acejs') || is_dir($dirname)) {
        drush_delete_dir('acejs', TRUE);
        drush_delete_dir($dirname, TRUE);
        drush_log(dt('An existing ACE JS plugin was deleted from @path', array('@path' => $path)), 'notice');
      }
      
      // Decompress the zip archive
      if (drush_tarball_extract($filename, $path)) {
        // Change the directory name to "openseadragon" if needed
          drush_move_dir($dirname, 'acejs', TRUE);
          $dirname = 'acejs';
        if (is_dir($dirname)) {
          drush_log(dt('Ace JS plugin has been installed in @path', array('@path' => $filepath."/".$dirname)), 'success');
        }
        else {
          drush_log(dt('Drush was unable to install the Ace JS plugin to @path', array('@path' =>  $filepath."/".$dirname)), 'error');
        }
      }
      else {
        drush_log(dt('Drush was unable to extract the download Ace JS plugin to @path', array('@path' => $filepath)), 'error');
        //Extraction failed
      }  

      // Set working directory back to the previous working directory.
      chdir($olddir);
    }
  }
  else {
    drush_log(dt('Drush was unable to install the Ace JS plugin because @path is not writable.', array('@path' => $path)), 'warning');
  }
}