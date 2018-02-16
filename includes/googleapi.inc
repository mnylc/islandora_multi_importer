<?php

/**
 * @file
 * includes/googleapi.inc
 * 
 * Contains Remote Google Spreadsheet Parsing/Processing utilities 
 * @author Diego Pino Navarro
 */


/**
 * Returns an authorized API client.
 * @return Google_Client the authorized client object
 */
function islandora_multi_importer_googleclient() {
  // Yeah, this people don´t use namespaces.

  $client = new \Google_Client();
  $client->setApplicationName('Isladora Multi Importer');
  $client->setScopes(\Google_Service_Sheets::SPREADSHEETS_READONLY);
  // We won´t be using setAuthConfig here because
  // it depends on an actual file. We will make our 
  // own tiny one settings setter from DB.
  error_log(variable_get('islandora_multi_importer_googleClientID', ''));
  error_log(variable_get('islandora_multi_importer_googleClientSecret', ''));
  error_log(variable_get('islandora_multi_importer_googleToken', FALSE));
  
  $client->setClientId(variable_get('islandora_multi_importer_googleClientID', ''));
  $client->setClientSecret(variable_get('islandora_multi_importer_googleClientSecret', ''));
  $client->setRedirectUri("urn:ietf:wg:oauth:2.0:oob");
  $client->setDeveloperKey("AIzaSyCJs54GGXJMYkTv5gh5LawpZeTEFqXsDmA");

  $client->setAccessType('offline');
  //  $accessToken = $client->fetchAccessTokenWithAuthCode($authCode);
  return $client;
}


/**
 * Tries to use passed tokens array, if expired fetches new one
 *
 * @param string $refreshToken
 * @return array access token
 */
function islandora_multi_importer_googleclient_authwithtoken($client, $tokens) {
  //Tokens is the whole array in form of
  $client->setAccessToken($tokens);

// Refresh the token if expired.
  if ($client->isAccessTokenExpired()) {
    $tokens = $client->fetchAccessTokenWithRefreshToken($tokens['refresh_token']);
    if ($tokens && isset($tokens['access_token'])) {
      // Don't store wrong credentials!
       variable_set('islandora_multi_importer_googleToken', $tokens);
    }
    else {
      drupal_set_message(t('Google Error, probably your credentials expired'));
    }
  }
  return $client; 
}

function islandora_multi_importer_googleclient_connect() {

  $client = islandora_multi_importer_googleclient();
  $tokens = variable_get('islandora_multi_importer_googleToken', FALSE);
  error_log($tokens);
  if ($tokens) {
    // This should auth and refresh. 
    $client = islandora_multi_importer_googleclient_authwithtoken($client ,$tokens);
  }
  return $client;
}