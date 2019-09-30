#! /usr/bin/python

import sys
import os
import datetime

def main():
    mypath = sys.argv[1]

    print('Patching external links in doxygen docu')

    # Patch <a href to use target _blank
    htmlFiles = [os.path.join(mypath, f) for f in os.listdir(mypath) if (os.path.isfile(os.path.join(mypath, f)) and f.endswith('.html'))]

    for f in htmlFiles:
        data = open(f, 'r').read()

        data = data.replace('href="http', 'target="_blank" href="http')

        open(f, 'w').write(data)


    print('Removing empty markdown source files')
    [os.remove(os.path.join(mypath, f)) for f in os.listdir(mypath) if (os.path.isfile(os.path.join(mypath, f)) and   f.endswith('8md.html'))]

    print('Creating sitemap.xml')

    htmlFiles = [f for f in os.listdir(mypath) if (os.path.isfile(os.path.join(mypath, f)) and f.endswith('.html') and not f.endswith('source.html'))]

    xmlData = '<urlset xsi:schemaLocation="http://www.sitemaps.org/schemas/sitemap/0.9 http://www.sitemaps.org/schemas/sitemap/0.9/sitemap.xsd">\n'

    now = datetime.datetime.now()
    time = now.strftime("%Y-%m-%dT%H:%M:%S+00:00")

    for f in htmlFiles:
        xmlData += '  <url><loc>https://docs.cppinsights.io/%s</loc><lastmod>%s</lastmod><priority>1.00</priority></url>\n' %(f, time)

    xmlData += '</urlset>\n'

    open(os.path.join(mypath,'sitemap.xml'), 'w').write(xmlData)

    return

sys.exit(main())
