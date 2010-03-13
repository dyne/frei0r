#!/bin/sh

# this script generates the website configuration locally
# it requires emacs, texlive and latest stable org-mode
# will download some scripts and prepare this directory
# so that running make the website will be compiled

mkdir -p templates_c; chmod a+w templates_c
mkdir -p cache; chmod a+w cache
cat > apache.conf <<EOT
<VirtualHost *:80>
	ServerAdmin webmaster@localhost

	ServerName freej.local.org
	DocumentRoot `pwd`
	<Directory `pwd`/>
		Options Indexes FollowSymLinks MultiViews
		AllowOverride None
		Order allow,deny
		allow from all
	</Directory>

	ErrorLog /var/log/apache2/freej-error.log

	# Possible values include: debug, info, notice, warn, error, crit,
	# alert, emerg.
	LogLevel warn

	CustomLog /var/log/apache2/freej-access.log combined
</VirtualHost>
EOT
echo
echo "generation completed, link the apache.conf to enable the website"
echo "then add freej.local.org to your /etc/hosts to reach it:"
echo
echo "sudo ln -s `pwd`/apache.conf /etc/apache2/sites-enabled/freej-web"
echo "sudo sh -c 'echo \"127.0.0.1 freej.local.org\" >> /etc/hosts'"
echo
echo "use 'make' to generate a new version of the website which will be"
echo "visible on http://freej.local.org"
echo

