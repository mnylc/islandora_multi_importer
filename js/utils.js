/**
 * @file
 * Tiny ajax driven UI helpegs
 * @author Diego Pino Navarro
 */

/**
 * focus an horizontal tab
 */

$(function() {
Drupal.ajax.prototype.commands.focusHTab =  function(ajax, response, status) {
// response is what we pass from PHP
    console.log(ajax);
    console.log(response.arguments);
    $(response.arguments.tabid).data('horizontalTab').focus();
}
}
)(jQuery);