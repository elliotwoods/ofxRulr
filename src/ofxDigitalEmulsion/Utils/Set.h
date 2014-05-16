#pragma once

namespace ofxDigitalEmulsion {
	namespace Utils {
		template<typename BaseType>
		class Set : public vector<shared_ptr<BaseType> > {
		public:
			template<typename T>
			shared_ptr<T> get<T>()  {
				for(auto item : * this) {
					auto castItem = dynamic_pointer_cast<T>(item);
					if (cast != NULL) {
						return cast;
					}
				}
				ofLogError("ofxDigitalEmulsion") << "Item of type [" << T().getTypeName() << "] could not be found in Set<" << typeid(BaseType).name() << ">";
				return shared_ptr<T>();
			}
		};
	}
}