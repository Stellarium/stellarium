#ifndef MOBILEIMAGEPROVIDER_HPP
#define MOBILEIMAGEPROVIDER_HPP

#include <QDeclarativeImageProvider>
#include "SystemDisplayInfo.hpp"

// Acceptable extensions. Right now, only one for raster, one for vector
#define RASTER_POSTFIX ".png"
#define VECTOR_POSTFIX ".svg"

// These directories
#define LOW_DPI_PREFIX "gui/images-ldpi/"
#define MEDIUM_DPI_PREFIX "gui/images-mdpi/"
#define HIGH_DPI_PREFIX "gui/images-hdpi/"
#define XHIGH_DPI_PREFIX "gui/images-xhdpi/"
#define MISSING_IMAGE "gui/images/missing-image"

// This directory is intended for images which can be used at any
// (or any reasonable) scale, or are used as a fall-back in the case
// that we cannot find an image in the above directories. Vector images
// should be placed in this directory. These images will be scaled.
#define DEFAULT_DPI_PREFIX "gui/images/"

// This directory is also intended for images which can be used at any
// scale, but these are never scaled. This is the last directory checked.
#define NOSCALE_PREFIX "gui/images-nodpi/"

//! This class provides a simplified method of access for a QML GUI to
//! image files of varying sizes intended for screens of varying densities.
//! This roughly follows the idea utilized by Android for its own image
//! loading, but is cross-platform and works on the C++/Qt side without
//! some sort of horrible hacks.
//!
//! Once the provider has been registered to the QDeclarativeEngine, QML
//! files can access images through "image://ImageProviderName/search-icon"
//! The correct file for the given screen density will then be accessed,
//! for instance, "stellarium/gui/images-mdpi/search-icon.png"
//! Notice that the URL used from QML gives neither extension nor density.
//! The MobileImageProvider will determine the correct density directory
//! depending on the actual screen density, and will take the first
//! matching file in that directory, so long as the extension is supported.
//! Not requiring an extension allows us to, for instance, fall back to a
//! vector image if we can't find an appropriate raster image.
//!
//! File matching works as follows:
//! First, look at the DPI, and find the corresponding dpi directory.
//! (These are specified above as SOMETHING_DPI_PREFIX.)
//! Look in the corresponding directory. If the image is found, we're done.
//! If not, we need to look for the best match.
//! 1. Try the directory two buckets up. Low tries high, medium tries xhigh.
//!    the justification is that this is requires a 1/2 scaling, which
//!    works much better than an 0.75x scaling.
//! 2. Look in DEFAULT_DPI_PREFIX for a vector image
//! 3. Look in NOSCALE_PREFIX
//! 4. Try the directory one bucket up.
//! 5. Try the directory three buckets up (if there is one)
//! 6. Look in DEFAULT_DPI_PREFIX for a raster image
//! 7. Try the lower buckets, and scale the image up. Your GUI will be ugly.
//! 8. Display MISSING_IMAGE
//!
//! This fuctionality isn't actually in:
//!		We also take the requested size into account. If we find a hit in the
//!		current density directory, but the requested size is smaller/larger,
//!		we'll try looking for a better match depending on the scale of the image:
//!		1. If the scale isn't 0.5x, 0.75x, 1.5x, or 2x, check DEFAULT_DPI_PREFIX
//!		   for a vector image. If that fails,
//!		1a. Look up at larger buckets, starting at the next best and going up,
//!		    then going down. Give up if we come back to start.
//!		2. If the scale is a nice number, do the normal search again, but this
//!		   time pretend our screen is actually the other density
//!		2a. If the other density is out-of-range, try the DEFAULT directory for
//!		    a vector image. If that doesn't exist, run the search with the closest
//!		    density bucket (either LOW or XHIGH)
//!
//! Instead, just do the lazy thing: use the vector image, if one exists.
//! Otherwise, just scale the image.
//!
//! Images in the NOSCALE_PREFIX directory are special. These will
//! _never_ be scaled. You shouldn't include a file in NOSCALE_PREFIX under
//! the same name as a file in another directory unless you have a really
//! good reason that I can't foresee.
//!
//! Ideally, we want the search to end at step 1, 2, or 3. This means, we want to
//! have rasters in every DPI bucket and/or a vector image in the default dir.
//! We also want to always ask for the right size (or, have a vector image).
//! If it ever gets past 3 (or hits 1a/2a in the "Wrong size" case), this
//! class will shoot out a very stern warning message to the log.
class MobileImageProvider : public QDeclarativeImageProvider
{
public:
	MobileImageProvider(SystemDisplayInfo::DpiBucket bucket);

	//! @param id             the requested image source, with the "image:" scheme and provider identifier removed.
	//!                       If QML requests image://ImageProviderName/search-icon, the string is "search-icon"
	//! @param requestedsize  corresponds to the QML image element's sourceSize property. The image returned should
	//!                       be this size, unless it's an invalid size (sourceSize can be set to 'undefined' in
	//!                       QML to simply use the natural size of the image)
	//! @param size           will be set to the original size of the image; this will resize the Image element if
	//!                       its size wasn't already set
	//! @return               the requested image, at the requested size (if possible)
	QImage requestImage ( const QString & id, QSize * size, const QSize & requestedSize );

	//! Actually look for the image.
	//! Uses the strategy described in the class comments, but stops before 8.; instead, returns
	//! an empty string if it's unable to find anything.
	//!
	//! @param id            requested image source, as from requestImage
	//! @param requestedSize the requested size, again as from requestImage
	//! @param testBucket    the bucket we're going to base our search around (assume this is the screen's size).
	//!                      Iff testBucket == this.bucket we might try searching another if we hit case 2. of the
	//!                      "wrong size" scenario describe in the class comments above.
	//! @return              path to the image we wanted to find
	QString findPath(const QString & id, SystemDisplayInfo::DpiBucket testBucket);

	//! Convert a bucket to path
	//! @param pathBucket the bucket we want the path for
	//! @return           the bucket's directory
	QString getBucketPath(SystemDisplayInfo::DpiBucket pathBucket);

	//! Find a file using a StelFileMgr path (without the extension).
	//! @param stelPath the path to search in
	//! @param extension if not an empty string, check only for this extension
	QString findFile(QString stelPath, QString extension = "");

	//! Actually generate an image. Either load in a raster image, or render an SVG
	//! @param path the path to load the image from
	//! @return     an image corresponding to the file loaded
	QImage generateImage(QString path, const QSize &requestedSize);
private:
	//! The screen device's DPI bucket, wherein we search for an image
	SystemDisplayInfo::DpiBucket bucket;
};

#endif // MOBILEIMAGEPROVIDER_HPP
