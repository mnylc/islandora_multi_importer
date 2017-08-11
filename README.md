# islandora_multi_importer

### DCMNY admin documentation 

[Multi-Importer Admin Instructions for DCMNY Administrators] (https://docs.google.com/document/d/18oB6sX-8s6sIScgUf7RbkFFlJ52Y9k_f9FcsaWvDJ7s/edit?usp=sharing)

[Sample CSV (configured for multi-importer ingest)] (https://drive.google.com/file/d/0BzuVASmQStk8dWJ6UGt6bmphcGs/view?usp=sharing)

[DCMNY Metadata Spreadsheet (for use with multi-importer ingest)] (https://docs.google.com/spreadsheets/d/1fL9oO_x35tUx3wKSZ4a848ravc4Oh1Wjk5ykU3H1Ti8/edit?usp=sharing)

### Installation 
* Ensure that [composer](https://getcomposer.org/doc/00-intro.md#installation-linux-unix-osx) is installed in your enviornment: ```composer version```.  
* Clone the module into the modules folder: ```git clone https://github.com/mnylc/islandora_multi_importer```
* Install the module's dependency using the composer ```composer install```
* Ensure that you have all the module's drupal dependencies (ex [Field Group](https://www.drupal.org/project/field_group))
* Enable the module


### Twig

[Sample Twig Templates (for use with multi-importer ingest)] (https://github.com/mnylc/dcmny/tree/master/twig)

[Twig Documentation] (http://twig.sensiolabs.org/documentation)


## Why Use Multi-importer
* UI driven integrated workflow for ingest and update
* Metadata Cleanup: Export your MODS metadata as CSV via solr, clean up, then update the MODS datastream of the objects by recreating the MODS datastream using Twing
* To ingest different content types at the same time
* To avoid having to follow strict naming conventions and folder structure dictated by many Islandora batch ingest processes
* Selectively choose which derivatives you want to create and upload
* To avoid the OpenRefine/XSLT approach to creating MODS from CSVs
* To take advantage of the Twig Templating system for creating MODS from CSVs
* To preview the MODS output easily
* Supports integration with Google Spreadsheets, Zip/Local/Amazon storage
