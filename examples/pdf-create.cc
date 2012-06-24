#include <qpdf/QPDF.hh>
#include <qpdf/QPDFWriter.hh>
#include <qpdf/QPDFObjectHandle.hh>
#include <qpdf/QUtil.hh>
#include <iostream>
#include <string.h>
#include <stdlib.h>

static char const* whoami = 0;

// This is a simple StreamDataProvider that writes image data for an
// orange square of the given width and height.
class ImageProvider: public QPDFObjectHandle::StreamDataProvider
{
  public:
    ImageProvider(int width, int height);
    virtual ~ImageProvider();
    virtual void provideStreamData(int objid, int generation,
				   Pipeline* pipeline);
    size_t getLength() const;

  private:
    int width;
    int height;
};

ImageProvider::ImageProvider(int width, int height) :
    width(width),
    height(height)
{
}

ImageProvider::~ImageProvider()
{
}

void
ImageProvider::provideStreamData(int objid, int generation,
                                 Pipeline* pipeline)
{
    for (int i = 0; i < width * height; ++i)
    {
        pipeline->write((unsigned char*)"\xff\x7f\x00", 3);
    }
    pipeline->finish();
}

size_t
ImageProvider::getLength() const
{
    return 3 * width * height;
}

void usage()
{
    std::cerr << "Usage: " << whoami << " filename" << std::endl
	      << "Creates a simple PDF and writes it to filename" << std::endl;
    exit(2);
}

static QPDFObjectHandle createPageContents(QPDF& pdf, std::string const& text)
{
    // Create a stream that displays our image and the given text in
    // our font.
    std::string contents =
        "BT /F1 24 Tf 72 720 Td (" + text + ") Tj ET\n"
        "q 144 0 0 144 234 324 cm /Im1 Do Q\n";
    PointerHolder<Buffer> b = new Buffer(contents.length());
    unsigned char* bp = b->getBuffer();
    memcpy(bp, (char*)contents.c_str(), contents.length());
    return QPDFObjectHandle::newStream(&pdf, b);
}

QPDFObjectHandle newName(std::string const& name)
{
    return QPDFObjectHandle::newName(name);
}

QPDFObjectHandle newInteger(int val)
{
    return QPDFObjectHandle::newInteger(val);
}

static void create_pdf(char const* filename)
{
    QPDF pdf;

    // Start with an empty PDF that has no pages or non-required objects.
    pdf.emptyPDF();

    // Add an indirect object to contain a font descriptor for the
    // built-in Helvetica font.
    QPDFObjectHandle font = pdf.makeIndirectObject(
        QPDFObjectHandle::newDictionary());
    font.replaceKey("/Type", newName("/Font"));
    font.replaceKey("/Subtype", newName("/Type1"));
    font.replaceKey("/Name", newName("/F1"));
    font.replaceKey("/BaseFont", newName("/Helvetica"));
    font.replaceKey("/Encoding", newName("/WinAnsiEncoding"));

    // Create a stream to encode our image.  We don't have to set the
    // length or filters.  QPDFWriter will fill in the length and
    // compress the stream data using FlateDecode by default.
    QPDFObjectHandle image = QPDFObjectHandle::newStream(&pdf);
    QPDFObjectHandle image_dict = image.getDict();
    image_dict.replaceKey("/Type", newName("/XObject"));
    image_dict.replaceKey("/Subtype", newName("/Image"));
    image_dict.replaceKey("/ColorSpace", newName("/DeviceRGB"));
    image_dict.replaceKey("/BitsPerComponent", newInteger(8));
    image_dict.replaceKey("/Width", newInteger(100));
    image_dict.replaceKey("/Height", newInteger(100));
    // Provide the stream data.
    ImageProvider* p = new ImageProvider(100, 100);
    PointerHolder<QPDFObjectHandle::StreamDataProvider> provider(p);
    image.replaceStreamData(provider,
                            QPDFObjectHandle::newNull(),
                            QPDFObjectHandle::newNull(),
                            p->getLength());

    // Create direct objects as needed by the page dictionary.
    QPDFObjectHandle procset = QPDFObjectHandle::newArray();
    procset.appendItem(newName("/PDF"));
    procset.appendItem(newName("/Text"));
    procset.appendItem(newName("/ImageC"));

    QPDFObjectHandle rfont = QPDFObjectHandle::newDictionary();
    rfont.replaceKey("/F1", font);

    QPDFObjectHandle xobject = QPDFObjectHandle::newDictionary();
    xobject.replaceKey("/Im1", image);

    QPDFObjectHandle resources = QPDFObjectHandle::newDictionary();
    resources.replaceKey("/ProcSet", procset);
    resources.replaceKey("/Font", rfont);
    resources.replaceKey("/XObject", xobject);

    QPDFObjectHandle mediabox = QPDFObjectHandle::newArray();
    mediabox.appendItem(newInteger(0));
    mediabox.appendItem(newInteger(0));
    mediabox.appendItem(newInteger(612));
    mediabox.appendItem(newInteger(792));

    // Create the page content stream
    QPDFObjectHandle contents = createPageContents(
        pdf, "Look at the pretty, orange square!");

    // Create the page dictionary
    QPDFObjectHandle page = pdf.makeIndirectObject(
        QPDFObjectHandle::newDictionary());
    page.replaceKey("/Type", newName("/Page"));
    page.replaceKey("/MediaBox", mediabox);
    page.replaceKey("/Contents", contents);
    page.replaceKey("/Resources", resources);

    // Add the page to the PDF file
    pdf.addPage(page, true);

    // Write the results.  A real application would not call
    // setStaticID here, but this example does it for the sake of its
    // test suite.
    QPDFWriter w(pdf, filename);
    w.setStaticID(true);    // for testing only
    w.write();
}

int main(int argc, char* argv[])
{
    whoami = QUtil::getWhoami(argv[0]);

    // For libtool's sake....
    if (strncmp(whoami, "lt-", 3) == 0)
    {
	whoami += 3;
    }
    if (argc != 2)
    {
	usage();
    }
    char const* filename = argv[1];

    try
    {
	create_pdf(filename);
    }
    catch (std::exception& e)
    {
	std::cerr << e.what() << std::endl;
	exit(2);
    }

    return 0;
}