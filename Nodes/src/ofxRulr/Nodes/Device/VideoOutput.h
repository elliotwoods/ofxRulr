#pragma once

#include "ofxRulr.h"
#include "ofxCvGui/Panels/ElementHost.h"

#include <GLFW/glfw3.h>

namespace ofxRulr {
	namespace Nodes {
		namespace Device {
			class VideoOutput : public ofxRulr::Nodes::Base {
			public:
				class Output {
				public:
					Output(int index, GLFWmonitor *);
					int index;
					GLFWmonitor * monitor;
					string name;
					int width;
					int height;
				};

				VideoOutput();
				void init();
				string getTypeName() const override;
				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

				ofxCvGui::PanelPtr getView() override;
				void populateInspector(ofxCvGui::ElementGroupPtr);
				void update();

				float getWidth() const;
				float getHeight() const;
				ofRectangle getSize() const;
				ofRectangle getRectangleInCombinedOutput() const;

				int getVideoOutputSelection() const;
				const Output & getVideoOutputSelectionObject() const;
				void setVideoOutputSelection(int);
				int getVideoOutputCount() const;

				void applyNormalisedSplitViewTransform() const;

				void setWindowOpen(bool);
				bool isWindowOpen() const;

				/// Direct access drawing functions.
				/// (Only use if you know what you're doing).
				///{
				ofFbo & getFbo();
				GLFWwindow * getWindow() const;
				//
				void clearFbo(bool callDrawListeners);
				void begin();
				void end();
				void presentFbo();
				///}

				ofxLiquidEvent<ofRectangle> onDrawOutput;
			protected:
				void refreshMonitors();
				void createWindow();
				void destroyWindow();

				void calculateSplit();
				void callbackChangeSplit(float &);

				ofxCvGui::PanelPtr view;
				shared_ptr<ofxCvGui::Panels::ElementHost> monitorSelectionView;

				const GLFWvidmode * videoMode;

				vector<Output> videoOutputs;
				int videoOutputSelection;
				bool needsMonitorRefresh;

				GLFWwindow * window;

				ofParameter<bool> showWindow;
				ofParameter<float> splitHorizontal;
				ofParameter<float> splitVertical;
				ofParameter<float> splitUseIndex;
				ofParameter<int> testPattern; // 0 = none, 1 = grid, 2 = white

				ofFbo fbo;
				float width, height;
				bool scissorWasEnabled;

				ofxCvGui::PanelPtr fboPreview;
			};

			class MonitorEventChangeListener {
			public:
				MonitorEventChangeListener();
				ofxLiquidEvent<GLFWmonitor *> onMonitorChange;
			};

			extern MonitorEventChangeListener monitorEventChangeListener;
		}
	}
}