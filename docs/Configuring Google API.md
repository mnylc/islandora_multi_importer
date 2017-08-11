Islandora Multi Importer allows the csv data to be read directly from Google Spreadsheet.  You need to configure the Google API OAuth2 Credentials for the Multi Importer to enable the module to read the Google Spreadsheet.  The following are the steps to do that.

## Generating Google OAuth2 Credentials 
* Go to [Googles Developer Console](https://console.developers.google.com) and login.  It will take you to the API & Services Dashboard.  
![Developer Console](/docs/images/Developer_Console.png)

* If you have not created credentials or a project before, you would be promoted to create a Project.  Else, create a new Project or choose an existing project.  
![Create Project](/docs/images/Create_Project.png)

* Click the `Create credentials` select box and select `OAuth client ID`
![Create credentials select](/docs/images/Create_credentials.png)

* In the next screen, choose `Other` as the `Application type`
![Choose Other for Application type](/docs/images/Application_type.png)

* Copy the `client ID` and `client secret` into a text editor.  
![OAuth ID and Key](/docs/images/OAuth_ID_Key.png)

## Configuring the Google API in the moudule setting
* Go to the Multi Importer configuration page: `localhost:8000/admin/islandora/tools/multi_importer`.  Note that the configuration page would look slighltly different depending of if you are configuring it for the first time or updating the credentials.  
![Multi_Importer_Settings](/docs/images/Multi_Importer_Settings.png)

* Enter the `client id` in `Google API App ID` and `client secret` in `Google API App Secret Key` and save the form.  Note that when you save, the secret key field will blanked.

* You should see a third form element called `Google API Auth Code` and a link ("Enter from this URL").  Click the link. It will ask to Choose a Google account and login.  When loged in, it will request permission to "View your Google Spreadsheets".  Click `Allow`.

![Provide Permission](/docs/images/Provide_Permission.png)

* You will get a confirmation page from google and a token.  Put the token in the third form element, and save.
![Provide Permission](/docs/images/Token.png)

* To confirm, you can go the console and confirm the variable information.
```shell
vagrant@islandora:/var/www/drupal/sites$ drush vget islandora_multi_importer_googleClientID
islandora_multi_importer_googleClientID: '4090389731745-5kl8f2adtgv9c6qgokt51ixph9103lrl.apps.googleusercontent.com'

vagrant@islandora:/var/www/drupal/sites$ drush vget islandora_multi_importer_googleClientAuthCode
islandora_multi_importer_googleClientAuthCode: '4/l0a1bt1slTBhuT4RNlTOwnaIMibz8V3XX5aGQdfYK-s'

vagrant@islandora:/var/www/drupal/sites$ drush vget islandora_multi_importer_googleClientSecret
islandora_multi_importer_googleClientSecret: '1Gljf9mYooAIJ4rA7uZu8bwO'

vagrant@islandora:/var/www/drupal/sites$ drush vget islandora_multi_importer_googleClientToken
islandora_multi_importer_googleClientToken: array (
  'access_token' => 'ja45.FlukBDwllFb411AgbyrGeh8PedgFMSgsPo8IFi8k1IA1BtOeu4HrpcYf7kXC1T3K8CMy-SO10Ds8YuelTSAZQ6rv9cjP0S2jo5DIi_q83nxw3eZNu6xdO6GniDvFW',
  'token_type' => 'Bearer',
  'expires_in' => 3600,
  'refresh_token' => '4/AIceqIe95yAMAIn_VdfhCqaIXiZ12I5Tt-iZCqLgA-w',
  'created' => 1502461728,
)
```



