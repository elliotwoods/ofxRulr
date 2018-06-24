#include "pch_Plugin_Experiments.h"
#include "planesToPoint.h"

class Model : public ofxNonLinearFit::Models::Base<ofxRay::Plane, Model> {
public:
	unsigned int getParameterCount() const {
		return 3;
	}

	void getResidual(ofxRay::Plane plane, double & residual, double * gradient = 0) const {
		if (gradient) {
			throw(ofxRulr::Exception("not implemented"));
		}

		residual = plane.getDistanceTo(this->point);
	};

	void evaluate(ofxRay::Plane &) const {
		//nothing to do
	}

	void cacheModel() {
		this->point.x = this->parameters[0];
		this->point.y = this->parameters[1];
		this->point.z = this->parameters[2];
	}

	ofVec3f point;
};

ofVec3f planesToPoint(const vector<ofxRay::Plane> & planes) {
	ofxNonLinearFit::Fit<Model> fit;

	Model model;
	double residual;
	fit.optimise(model, &planes, &residual);
	return model.point;
}