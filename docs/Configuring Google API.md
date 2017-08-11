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
