import urllib2

URL = 'http://exoplanet.eu/catalog/csv/'
CSV = './exoplanets.csv'

req = urllib2.Request(URL)
req.add_header('User-Agent', 'Mozilla/5.0 (Stellarium Exoplanets Catalog Updater 3.0; http://stellarium.org)')
req.add_header('Referer', 'http://exoplanet.eu/catalog/#')

try:
    response = urllib2.urlopen(req)
except urllib2.HTTPError as e:
    print 'The server couldn\'t fulfill the request.'
    print 'Error code: ', e.code
except urllib2.URLError as e:
    print 'We failed to reach a server.'
    print 'Reason: ', e.reason
else:
    csvdata = response.read()
    datafile = open(CSV, 'w')
    datafile.write(csvdata)
    datafile.close()
