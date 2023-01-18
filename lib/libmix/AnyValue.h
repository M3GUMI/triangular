#pragma once

#include <typeinfo>
#include <algorithm>
#include <assert.h>


class AnyValue
{
	class ValueHolder {
	public:
		virtual ~ValueHolder() {}
		virtual const std::type_info& type() const = 0;
		virtual ValueHolder* clone() const = 0;
	};

	template<typename T>
	class Value : public ValueHolder
	{
	public:
		Value(const T& v) : held_(v) {}

		const std::type_info& type() const {
			return typeid(T);
		}

		ValueHolder* clone() const {
			return new Value<T>(held_);
		}

		T held_;
	};
public:
	AnyValue() : content_(nullptr){}

	~AnyValue() {
		delete content_;
	}

	template<typename T>
	AnyValue(const T& v) {
		content_ = new Value<T>(v);
	}

	AnyValue(const AnyValue& other) {
		content_ = other.content_ ? other.content_->clone() : nullptr;
	}

	AnyValue& operator=(const AnyValue& other) {
		delete content_;
		content_ = other.content_ ? other.content_->clone() : nullptr;
		return *this;
	}
#if (__cplusplus >= 201103L || _MSC_VER >= 1900)
	AnyValue(AnyValue&& other) {
		content_ = other.content_;
		other.content_ = nullptr;
	}

	AnyValue& operator=(AnyValue&& other) {
		std::swap(content_, other.content_);
		return *this;
	}
#endif
	template<typename T>
	AnyValue& operator=(const T& v){
		delete content_;
		content_ = new Value<T>(v);
		return *this;
	}

	const std::type_info& type() const{
		return content_ ? content_->type() : typeid(void);
	}

	template<typename T>
	T& get() {
		assert(typeid(T) == content_->type());
		return ((Value<T>*)content_)->held_;
	}

	template<typename T>
	const T& get() const {
		assert(typeid(T) == content_->type());
		return static_cast<Value<T>*>(content_)->held_;
	}

private:
	ValueHolder * content_;
};