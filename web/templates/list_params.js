// freej script to enumerate frei0r parameters in html

fr = new Filter("%filter_name%");
if(!fr) quit();


echo("<h3>" + fr.description + "</h3>");
echo("<h4>by: " + fr.author + "</h4>");


// PRINT OUT PARAMETERS
params = fr.parameters;
echo("<div id=\"parameters\">");
if(params.length) {
    echo("<h3>Parameters:</h3>");
    echo("<ul>");
    for(c=0; c<params.length; c++) {
	echo("<li>" + c + ") <i>" + params[c].type
	     + "</i> : " + params[c].name
	     + " :: " + params[c].description + "</li>"); 
    }
    echo("</ul>");
} else {
    echo("<h4>This effect has no parameters</h4>");
}
echo("</div>");
    
quit();
