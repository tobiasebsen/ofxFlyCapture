#include "ofxFlyCapture.h"
#include "FlyCapture2.h"

using namespace FlyCapture2;

static const std::pair<PropertyType,string> propNamePairs[] = {
	{BRIGHTNESS, "Brightness"},
	{AUTO_EXPOSURE, "Exposure"},
	{SHARPNESS,"Sharpness"},
	{WHITE_BALANCE,"White Balance"},
	{HUE,"Hue"},
	{SATURATION,"Saturation"},
	{GAMMA,"Gamma"},
	{IRIS,"Iris"},
	{FOCUS,"Focus"},
	{ZOOM,"Zoom"},
	{PAN,"Pan"},
	{TILT,"Tilt"},
	{SHUTTER,"Shutter"},
	{GAIN,"Gain"},
	{TRIGGER_MODE,"Trigger Mode"},
	{TRIGGER_DELAY,"Trigger Delay"},
	{FRAME_RATE,"Frame Rate"},
	{TEMPERATURE,"Temperature"},
};
static std::map<PropertyType,string> propNameMap(propNamePairs, propNamePairs + sizeof(propNamePairs)/sizeof(propNamePairs[0]));

ofxFlyCapture::ofxFlyCapture() : 
	bChooseDevice(false),
	bGrabberInitied(false)
{
}


ofxFlyCapture::~ofxFlyCapture()
{
}

vector<ofVideoDevice> ofxFlyCapture::listDevices(bool fillFormats) {
	std::vector<ofVideoDevice> devices;

	unsigned int numCameras = 0;
	BusManager busMgr;
	auto error = busMgr.GetNumOfCameras(&numCameras);
	if (error != PGRERROR_OK)
	{
		ofLogError() << error.GetDescription();
		return devices;
	}

	for (auto i = 0u; i < numCameras; i++)
	{
		PGRGuid guid;
		error = busMgr.GetCameraFromIndex(i, &guid);
		if (error != PGRERROR_OK)
		{
			ofLogError() << error.GetDescription();
			break;
		}
		
		Camera c;
		if (c.Connect(&guid) == PGRERROR_OK) {
			CameraInfo ci;
			c.GetCameraInfo(&ci);
			ofVideoDevice d;
			d.id = i;
			d.deviceName = ci.modelName;
			d.hardwareName = ci.modelName;
			d.serialID = ofToString(ci.serialNumber);
			d.bAvailable = true;
			// todo: fill formats property

			devices.push_back(d);
			c.Disconnect();
		}
	}

	return devices;
}

std::tuple<int, int, ofPixelFormat> parseVideoMode(VideoMode vm) {
	switch (vm) {
	case VIDEOMODE_640x480RGB:
		return make_tuple<int, int, ofPixelFormat>(640, 480, OF_PIXELS_RGB);
	case VIDEOMODE_640x480Y8:
		return make_tuple<int, int, ofPixelFormat>(640, 480, OF_PIXELS_GRAY);
	case VIDEOMODE_800x600RGB:
		return make_tuple<int, int, ofPixelFormat>(800, 600, OF_PIXELS_RGB);
	case VIDEOMODE_800x600Y8:
		return make_tuple<int, int, ofPixelFormat>(800, 600, OF_PIXELS_GRAY);
	case VIDEOMODE_1024x768RGB:
		return make_tuple<int, int, ofPixelFormat>(1024, 768, OF_PIXELS_RGB);
	case VIDEOMODE_1024x768Y8:
		return make_tuple<int, int, ofPixelFormat>(1024, 768, OF_PIXELS_GRAY);
	case VIDEOMODE_1280x960RGB:
		return make_tuple<int, int, ofPixelFormat>(1280, 960, OF_PIXELS_RGB);
	case VIDEOMODE_1280x960Y8:
		return make_tuple<int, int, ofPixelFormat>(1280, 960, OF_PIXELS_GRAY);
	case VIDEOMODE_1600x1200RGB:
		return make_tuple<int, int, ofPixelFormat>(1600, 1200, OF_PIXELS_RGB);
	case VIDEOMODE_1600x1200Y8:
		return make_tuple<int, int, ofPixelFormat>(1600, 1200, OF_PIXELS_GRAY);
	default:
		return make_tuple<int, int, ofPixelFormat>(0, 0, OF_PIXELS_GRAY);
	}
}

bool ofxFlyCapture::setup(int w, int h, float speedPct) {
	camera = make_shared<Camera>();
	if (bChooseDevice) {
		BusManager busMgr;
		PGRGuid guid;
		busMgr.GetCameraFromIndex(deviceID, &guid);
		if (camera->Connect(&guid) != PGRERROR_OK) {
			return false;
		}
	}
	else {
		if (camera->Connect() != PGRERROR_OK) {
			return false;
		}
	}

	// todo set frame size to desired (w, h)
	/*VideoMode vm;
	FrameRate fr;
	camera->GetVideoModeAndFrameRate(&vm, &fr);
	if (vm == VIDEOMODE_FORMAT7 && fr == FRAMERATE_FORMAT7) {
		Format7ImageSettings fmt7is;
		unsigned int ps = 0;
		float pct = 0;
		camera->GetFormat7Configuration(&fmt7is, &ps, &pct);
		width = fmt7is.width;
		height = fmt7is.height;
		switch (fmt7is.pixelFormat) {
		case PIXEL_FORMAT_MONO8:
			pixelFormat = OF_PIXELS_GRAY;
			break;
		case PIXEL_FORMAT_RGB8:
			pixelFormat = OF_PIXELS_RGB;
			break;

		default:
			ofLogWarning() << "unknown pixel format. " << fmt7is.pixelFormat;
		}
	}
	else {
		auto fmt = parseVideoMode(vm);
		width = std::get<0>(fmt);
		height = std::get<1>(fmt);
		pixelFormat = std::get<2>(fmt);
	}*/

	Format7Info fmt7inf;
	bool supported = false;
	camera->GetFormat7Info(&fmt7inf, &supported);
	w = (w / fmt7inf.imageHStepSize) * fmt7inf.imageHStepSize;
	h = (h / fmt7inf.imageVStepSize) * fmt7inf.imageVStepSize;
	if (w > fmt7inf.maxWidth)
		w = fmt7inf.maxWidth;
	if (h > fmt7inf.maxHeight)
		h = fmt7inf.maxHeight;

	Format7ImageSettings fmt7is;
	unsigned int ps = 0;
	float pct = 0;
	camera->GetFormat7Configuration(&fmt7is, &ps, &pct);
	fmt7is.width = w;
	fmt7is.height = h;
	fmt7is.mode = MODE_0;

	bool valid = false;
	Format7PacketInfo fmt7pi;
	camera->ValidateFormat7Settings(&fmt7is, &valid, &fmt7pi);
	if (valid) {
		camera->SetFormat7Configuration(&fmt7is, speedPct);
		width = w;
		height = h;
		pixelFormat = fmt7is.pixelFormat == PIXEL_FORMAT_MONO8 ? OF_PIXELS_MONO : OF_PIXELS_RGB;
	}
	else
		return false;

	bGrabberInitied = camera->StartCapture() == PGRERROR_OK;
	return bGrabberInitied;
}

float ofxFlyCapture::getHeight() const {
	return height;
}

float ofxFlyCapture::getWidth() const {
	return width;
}

void ofxFlyCapture::setDeviceID(int deviceID) {
	this->deviceID = deviceID;
	bChooseDevice = true;
}

bool ofxFlyCapture::isFrameNew() const {
	return bIsFrameNew;
}

void ofxFlyCapture::close() {
	if (camera != nullptr) {
		camera->Disconnect();
		camera.reset();
		bGrabberInitied = false;
	}
}

bool ofxFlyCapture::isInitialized() const {
	return bGrabberInitied;
}

bool ofxFlyCapture::setPixelFormat(ofPixelFormat pixelFormat) {
	switch (pixelFormat)
	{
	case OF_PIXELS_GRAY:
	case OF_PIXELS_RGB:
		return this->pixelFormat == pixelFormat;
		
	default:
		ofLogWarning() << "setPixelFormat(): requested pixel format not supported";
		return false;
	}
}

ofPixelFormat ofxFlyCapture::getPixelFormat() const {
	return pixelFormat;
}

ofPixels & ofxFlyCapture::getPixels() {
	return pixels;
}

const ofPixels & ofxFlyCapture::getPixels() const {
	return pixels;
}

void ofxFlyCapture::update() {
	if (!camera) {
		return;
	}

	/*if (!pixels.isAllocated()
		|| pixels.getWidth() != width
		|| pixels.getHeight() != height
		|| pixels.getPixelFormat() != pixelFormat) {
		pixels.allocate(width, height, pixelFormat);
	}*/

	if (!tmpBuffer)
		tmpBuffer = make_shared<FlyCapture2::Image>();

	Error e = camera->RetrieveBuffer(tmpBuffer.get());
	if (e != PGRERROR_OK) {
		ofLogError() << e.GetDescription();
		bIsFrameNew = false;
	}
	else {
		TimeStamp ts = tmpBuffer->GetTimeStamp();
		if (ts.microSeconds != microSeconds) {
			//pixels.setFromPixels(tmpBuffer.GetData(), width, height, pixelFormat);
			pixels.setFromExternalPixels(tmpBuffer->GetData(), width, height, pixelFormat);
			bIsFrameNew = true;
			microSeconds = ts.microSeconds;
		}
		else
			bIsFrameNew = false;
	}
}

ofParameterGroup ofxFlyCapture::getProperties() {
	CameraInfo ci;
	camera->GetCameraInfo(&ci);

	ofParameterGroup group;
	group.setName(ofToString(ci.serialNumber));

	for (auto i = 0u; i < PropertyType::UNSPECIFIED_PROPERTY_TYPE; i++) {
		PropertyInfo info;
		info.type = (PropertyType)i;
		if (camera->GetPropertyInfo(&info) != PGRERROR_OK)
			continue;

		if (!info.present)
			continue;

		Property prop;
		prop.type = (PropertyType)i;
		camera->GetProperty(&prop);

		string & name = propNameMap[info.type];
		if (info.absValSupported) {
			ofParameter<float> p;
			p.setName(name);
			p.setMin(info.absMin);
			p.setMax(info.absMax);
			p.set(prop.absValue);
			group.add(p);
		}
		else if (info.min != info.max) {
			ofParameter<int> p;
			p.setName(name);
			p.setMin(info.min);
			p.setMax(info.max);
			p.set(prop.valueA);
			group.add(p);
		}
		if (info.autoSupported) {
			ofParameter<bool> p;
			p.setName("Auto " + name);
			p.set(prop.autoManualMode);
			group.add(p);
		}
		if (info.onOffSupported) {
			ofParameter<bool> p;
			p.setName("On/Off " + name);
			p.set(prop.onOff);
			group.add(p);
		}
	}
	return group;
}

void ofxFlyCapture::getProperties(ofParameterGroup & props) {
	for (int i=0; i<props.size(); i++) {
		getProperty(props[i]);
	}
}

Property ofxFlyCapture::getProperty(string name) {
	Property prop;
	prop.type = PropertyType::UNSPECIFIED_PROPERTY_TYPE;
	for (auto & pair : propNameMap) {
		if (pair.second == name) {
			prop.type = pair.first;
			break;
		}
	}
	if (prop.type != UNSPECIFIED_PROPERTY_TYPE)
		camera->GetProperty(&prop);
	return prop;
}

void ofxFlyCapture::getProperty(ofAbstractParameter & ap) {
	string name = ap.getName();
	bool isAuto = name.find("Auto ") == 0;
	bool isOnOff = name.find("On/Off ") == 0;

	if (isAuto)
		name = name.substr(5);
	if (isOnOff)
		name = name.substr(7);

	Property prop = getProperty(name);
	if (prop.type != UNSPECIFIED_PROPERTY_TYPE) {

		if (ap.type() == typeid(ofParameter<bool>).name()) {
			ofParameter<bool> & p = ap.cast<bool>();
			if (isAuto)
				p.set(prop.autoManualMode);
			if (isOnOff)
				p.set(prop.onOff);
		}
		else if (ap.type() == typeid(ofParameter<float>).name()) {
			ofParameter<float> & p = ap.cast<float>();
			p.set(prop.absValue);
		}
		else if (ap.type() == typeid(ofParameter<int>).name()) {
			ofParameter<int> & p = ap.cast<int>();
			p.set(prop.valueA);
		}
	}
}

void ofxFlyCapture::setProperty(ofAbstractParameter & ap) {
	string name = ap.getName();
	bool isAuto = name.find("Auto ") == 0;
	bool isOnOff = name.find("On/Off ") == 0;

	if (isAuto)
		name = name.substr(5);
	if (isOnOff)
		name = name.substr(7);

	Property prop = getProperty(name);
	if (prop.type != UNSPECIFIED_PROPERTY_TYPE) {

		if (ap.type() == typeid(ofParameter<bool>).name()) {
			ofParameter<bool> & p = ap.cast<bool>();
			if (isAuto)
				prop.autoManualMode = p.get();
			if (isOnOff)
				prop.onOff = p.get();
			camera->SetProperty(&prop);
		}
		else if (ap.type() == typeid(ofParameter<float>).name()) {
			ofParameter<float> & p = ap.cast<float>();
			prop.absControl = true;
			prop.absValue = p.get();
			camera->SetProperty(&prop);
		}
		else if (ap.type() == typeid(ofParameter<int>).name()) {
			ofParameter<int> & p = ap.cast<int>();
			prop.valueA = p.get();
			camera->SetProperty(&prop);
		}
	}
}

ofxFlyCaptureGrabber::ofxFlyCaptureGrabber() {
	this->setGrabber(make_shared<ofxFlyCapture>());
}