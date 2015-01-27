#pragma once

#include "ofxDigitalEmulsion.h"
#include "ofxCvGui/Panels/ElementHost.h"

#include <GLFW/glfw3.h>

namespace ofxDigitalEmulsion {
	namespace Device {
		class ProjectorOutput : public ofxDigitalEmulsion::Graph::Node {
		public:
			struct Output {
				string name;
				int width;
				int height;
				GLFWmonitor * monitor;
			};

			ProjectorOutput();
			void init() override;
			string getTypeName() const override;
			void serialize(Json::Value &);
			void deserialize(const Json::Value &);

			ofxCvGui::PanelPtr getView() override;
			void populateInspector(ofxCvGui::ElementGroupPtr);
			void update();

			ofFbo & getFbo();
			GLFWwindow * getWindow() const;
			ofRectangle getProjectorSize() const;
			float getWidth() const;
			float getHeight() const;
			ofRectangle getSplitSelectionRect() const;
			void applyNormalisedSplitViewTransform() const;

			void setWindowOpen(bool);
			bool isWindowOpen() const;
		protected:
			void refreshMonitors();
			void createWindow();
			void destroyWindow();

			void calculateSplit();
			void callbackChangeSplit(float &);

			shared_ptr<ofxCvGui::Panels::ElementHost> view;

			GLFWmonitor ** monitors;
			const GLFWvidmode * videoMode;

			int monitorCount;
			int monitorSelection;
			bool needsMonitorRefresh;

			GLFWwindow * window;

			ofParameter<bool> showWindow;
			ofParameter<bool> showTestGrid;
			ofParameter<float> splitHorizontal;
			ofParameter<float> splitVertical;
			ofParameter<float> splitUseIndex;

			ofFbo fbo;
			float width, height;
			vector<Output> cachedOutputs;
		};

		class MonitorEventChangeListener {
		public:
			MonitorEventChangeListener();
			ofxLiquidEvent<GLFWmonitor *> onMonitorChange;
		};

		extern MonitorEventChangeListener monitorEventChangeListener;
	}
}