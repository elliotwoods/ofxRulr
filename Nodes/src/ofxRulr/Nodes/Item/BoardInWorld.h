#pragma once

#include "RigidBody.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			class BoardInWorld : public RigidBody {
			public:
				BoardInWorld();
				string getTypeName() const override;
				bool findBoard(cv::Mat, vector<cv::Point2f>& imagePoints
					, vector<cv::Point3f>& objectPoints
					, vector<cv::Point3f>& worldPoints
					, FindBoardMode findBoardMode, cv::Mat cameraMatrix = cv::Mat(), cv::Mat distortionCoefficients = cv::Mat()) const;

				vector<glm::vec3> getAllWorldPoints() const;
			protected:
				void init();
				void update();
			};
		}
	}
}