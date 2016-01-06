#pragma once

#include "ofxRulr/Nodes/Base.h"

#include "ofxCvGui/Panels/Draws.h"
#include "ofxMachineVision/Grabber/Simple.h"

#include "of3dPrimitives.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Test {
			class Focus : public Nodes::Base {
			public:
				Focus();
				string getTypeName() const override;
				
				void init();
				void update();
				
				void populateInspector(ofxCvGui::InspectArguments &);
				void serialize(Json::Value &);
				void deserialize(const Json::Value &);
				
				ofxCvGui::PanelPtr getView() override;
				
				bool getRunFinderEnabled() const;
			protected:
				void connect(shared_ptr<ofxMachineVision::Grabber::Simple> grabber);
				void disconnect(shared_ptr<ofxMachineVision::Grabber::Simple> grabber);
				
				void calculateFocus(shared_ptr<ofxMachineVision::Frame> frame);
				
				void updateProcessSettings();
				
				ofxCvGui::PanelPtr view;
				ofxCvGui::ElementPtr widget;
				
				ofMutex lockPreview;
				
				struct {
					bool enabled;
					int blurSize;
					float highValue;
					float lowValue;
				} processSettings;
				mutex processSettingsMutex;
				
				struct {
					ofPixels grayscale;
					ofPixels blurred;
					ofPixels edges;
					
					int width = 0;
					int height = 0;
				} process; //only accessed in frame callbacks
				
				struct {
					ofImage highFrequency;
					ofImage lowFrequency;
					
					int width = 0;
					int height = 0;
					
					double value = 0.0;
					bool isFrameNew = false;
				} result;
				mutex resultMutex;
				
				ofFbo preview;
				
				// 0 = when selected
				// 1 = always
				ofParameter<int> activewhen;
				ofParameter<int> blurSize;
				ofParameter<float> highValue;
				ofParameter<float> lowValue;
			};
		}
	}
}