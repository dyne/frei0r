
fr = new Filter("%filter_name%");
if(!fr) quit();

// echo("loaded: " + fr.name);
params = fr.list_parameters();

if(params.length) {
    echo("<div id=\"parameters\">");
    echo("<h3>Parameters:</h3>");
    echo("<ul>");
    for(c=0; c<params.length; c++) {
	echo("<li>" + c + ". " + params[c] + "</li>"); 
    }
    echo("</ul>");
    echo("</div>");
}
quit();
