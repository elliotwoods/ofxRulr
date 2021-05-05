#include "pch_RulrCore.h"
#include "oF.h"

namespace ofxRulr {
	namespace Utils {

		//--
		// GLM
		//--
		//
#define DECLARE_SERIALIZE_GLM_VEC(TYPE) \
		void serialize(nlohmann::json& json, const TYPE& var) { \
			for (int i = 0; i < var.length(); i++) { \
				serialize(json[i], var[i]); \
			} \
		} \
		bool deserialize(const nlohmann::json& json, TYPE& var) { \
			if(json.is_array() && json.size() == var.length()) { \
				for (int i = 0; i < var.length(); i++) { \
					deserialize(json[i], var[i]); \
				} \
				return true; \
			} \
			else { \
				return false; \
			} \
		}

		#define DECLARE_SERIALIZE_GLM_MAT(TYPE) \
		void serialize(nlohmann::json& json, const TYPE& var) { \
			for (int i = 0; i < var.length(); i++) { \
				for(int j = 0; j<var[i].length();j++) { \
					serialize(json[i][j], var[i][j]); \
				} \
			} \
		} \
		bool deserialize(const nlohmann::json& json, TYPE& var) { \
			if(json.is_array() && json.size() == 4) { \
				for (int i = 0; i < var.length(); i++) { \
					for (int j = 0; j < var[i].length(); j++) {	\
						deserialize(json[i][j], var[i][j]); \
					} \
				} \
				return true; \
			} \
			else { \
				return false; \
			} \
		}

		DECLARE_SERIALIZE_GLM_VEC(glm::vec2)
		DECLARE_SERIALIZE_GLM_VEC(glm::vec3)
		DECLARE_SERIALIZE_GLM_VEC(glm::vec4)
		DECLARE_SERIALIZE_GLM_MAT(glm::mat3)
		DECLARE_SERIALIZE_GLM_MAT(glm::mat4)
		DECLARE_SERIALIZE_GLM_VEC(glm::quat)
		//
		//--



		//--
		// OpenCV
		//--
		//
		#define DECLARE_SERIALIZE_OPENCV_VEC(TYPE) \
		void serialize(nlohmann::json & json, const TYPE& var) { \
			serialize(json, ofxCv::toOf(var)); \
		} \
		bool deserialize(const nlohmann::json& json, TYPE& var) { \
			return deserialize(json, ofxCv::toOf(var)); \
		}
		DECLARE_SERIALIZE_OPENCV_VEC(cv::Point2f);
		DECLARE_SERIALIZE_OPENCV_VEC(cv::Point3f);
		//--




		//--
		// openFrameworks
		//--
		//

		//----------
		void 
		serialize(nlohmann::json& json, const ofMesh& mesh)
		{
			serialize(json["vertices"], mesh.getVertices());
			serialize(json["texCoords"], mesh.getTexCoords());
			serialize(json["colors"], mesh.getColors());
			serialize(json["indices"], mesh.getIndices());
			serialize(json["normals"], mesh.getNormals());
		}
		
		//----------
		bool
		deserialize(const nlohmann::json& json, ofMesh& mesh)
		{
			bool success = false;
			success |= deserialize(json["vertices"], mesh.getVertices());
			success |= deserialize(json["texCoords"], mesh.getTexCoords());
			success |= deserialize(json["colors"], mesh.getColors());
			success |= deserialize(json["indices"], mesh.getIndices());
			success |= deserialize(json["normals"], mesh.getNormals());
			return success;
		}
		
		//----------
		void
		serialize(nlohmann::json& json, const ofRectangle& rectangle)
		{
			serialize(json, "x", rectangle.x);
			serialize(json, "y", rectangle.y);
			serialize(json, "width", rectangle.width);
			serialize(json, "height", rectangle.height);
		}
		
		//----------
		bool
		deserialize(const nlohmann::json& json, ofRectangle& rectangle)
		{
			bool success = false;
			success |= deserialize(json, "x", rectangle.x);
			success |= deserialize(json, "y", rectangle.y);
			success |= deserialize(json, "width", rectangle.width);
			success |= deserialize(json, "height", rectangle.height);
			return success;
		}

		//----------
		template<typename Type>
		void _serialize(nlohmann::json& json, const ofColor_<Type>& value)
		{
			for (int i = 0; i < 4; i++) {
				serialize(json[i], value.v[i]);
			}
		}

		//----------
		template<typename Type>
		bool _deserialize(const nlohmann::json& json, ofColor_<Type>& value)
		{
			if (json.is_array()) {
				for (int i = 0; i < 4; i++) {
					deserialize(json[i], value.v[i]);
				}
				return true;
			}
			else {
				return false;
			}
		}

		DEFINE_SERIALIZE_VAR(ofColor_<uint8_t>);
		DEFINE_SERIALIZE_VAR(ofColor_<uint16_t>);
		DEFINE_SERIALIZE_VAR(ofColor_<float>);

	}
}