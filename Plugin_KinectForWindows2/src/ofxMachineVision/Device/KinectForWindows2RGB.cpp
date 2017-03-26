#include "pch_Plugin_KinectForWindows2.h"

#include "KinectForWindows2RGB.h"
#include "ofAppGLFWWindow.h"

namespace ofxMachineVision {
	namespace Device {
		//----------
		KinectForWindows2RGB::KinectForWindows2RGB() {
			this->frameIndex = 0;
		}

		//----------
		string KinectForWindows2RGB::getTypeName() const {
			return "KinectForWindows2RGB";
		}

		//----------
		Specification KinectForWindows2RGB::open(shared_ptr<Base::InitialisationSettings> initialisationSettings) {
			this->kinect = make_shared<ofxKinectForWindows2::Device>();

			try {
				this->kinect->open();
				this->kinect->initColorSource();

				if (!this->kinect->isOpen()) {
					auto message = "Cannot open Kinect";
					throw(Exception(message));
				}

				this->frameIndex = 0;

				//wait for the first live view frame to arrive
				auto startTime = ofGetElapsedTimeMillis();
				while (!this->kinect->getColorSource()->isFrameNew()) {
					if (ofGetElapsedTimeMillis() - startTime > 5000) {
						throw(Exception("Timeout getting first frame"));
					}
					ofSleepMillis(10);
					this->kinect->update();
				}
			}
			catch (Exception e) {
				ofLogError("ofxMachineVision::Device::KinectForWindows2RGB::open") << e.what();
				return Specification();
			}

			//copy the specification from the first frame
			const auto & pixels = this->kinect->getColorSource()->getPixels();
			Specification specification(CaptureSequenceType::Continuous
				, pixels.getWidth()
				, pixels.getHeight()
				, "Microsoft"
				, "Kinect2");

			{
				auto settings = dynamic_pointer_cast<KinectForWindows2RGB::InitialisationSettings>(initialisationSettings);
				if (!settings->color) {
					this->useColor = false;
					this->kinect->getColorSource()->setYuvPixelsEnabled(true);
					this->kinect->getColorSource()->setRgbaPixelsEnabled(false);

				}
			}
			return specification;
		}

		//----------
		void KinectForWindows2RGB::close() {
			if (this->kinect) {
				this->kinect->close();
				this->kinect.reset();
			}
		}

		//----------
		bool KinectForWindows2RGB::startCapture() {
			this->startTime = chrono::high_resolution_clock::now();
			return true;
		}

		//----------
		void KinectForWindows2RGB::stopCapture() {

		}

		//----------
		void KinectForWindows2RGB::updateIsFrameNew() {
			this->kinect->update();
		}

		//----------
		bool KinectForWindows2RGB::isFrameNew() {
			return this->kinect->isFrameNew();
		}

		//----------
		shared_ptr<Frame> KinectForWindows2RGB::getFrame() {
			shared_ptr<Frame> frame;

			if (this->useColor) {
				const auto & rgbPixels = this->kinect->getColorSource()->getPixels();
				frame = FramePool::X().getAvailableFrameFilledWith(rgbPixels);
			}
			else {
				const auto & yuvPixels = this->kinect->getColorSource()->getYuvPixels();
				frame = FramePool::X().getAvailableAllocatedFrame(yuvPixels.getWidth(), yuvPixels.getHeight(), ofPixelFormat::OF_PIXELS_GRAY);
				auto & pixels = frame->getPixels();
				
				auto in = yuvPixels.getData();
				auto out = pixels.getData();

				// YUY2 -> Y
				for (size_t i = 0; i < pixels.size(); i++) {
					out[i] = in[i * 2];
				}
			}

			frame->setTimestamp(chrono::high_resolution_clock::now() - this->startTime);
			frame->setFrameIndex(this->frameIndex++);
			
			return frame;
		}

		//----------
		shared_ptr<ofxKinectForWindows2::Device> KinectForWindows2RGB::getKinect() {
			return this->kinect;
		}
	}
}