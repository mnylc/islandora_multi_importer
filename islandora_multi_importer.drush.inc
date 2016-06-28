<?php

/**
 * @file
 * Drush hooks.
 */

/**
 * Implements hook_drush_command().
 */
function islandora_multi_importer_drush_command() {
  $items = array();

  $items['islandora-multi-importer-zeroMQboot'] = array(
    'aliases' => array('imboot'),
    'description' => 'Spawn an Islandora Multi Importer ZeroMQ server/workers ensamble',
    'drupal dependencies' => array('islandora_multi_importer'),
    'callback' => 'drush_islandora_multi_importer_zeroMQboot',
    'examples' => array(
      'drush -v --user=admin islandora_multi_importer_zeroMQboot',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
  );
  return $items;
}


/**
 *
 * Builds a preprocessor, and passes it off to a preprocessor handler.
 */
function drush_islandora_multi_importer_zeroMQboot() {
  // XXX: Due to how Drush bootstrapping works, the connection may be created
  // without credentials (when your site's front page is
  // 'islandora/object/some:object', for example). Resetting to ensure a new
  // connection gets created should fix it.
  module_load_include('inc', 'islandora_multi_importer', 'includes/zeroMQ');
  module_load_include('inc', 'islandora_multi_importer', 'includes/zmsg');
  drupal_static_reset('islandora_get_tuque_connection');
  $connection = islandora_get_tuque_connection();
  $boot = islandora_multi_importer_zeroMQ_boot($connection);
  return;
}
