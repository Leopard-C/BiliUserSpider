#pragma once

#include <mutex>
#include <memory>

template<typename T>
class Singleton {
public:
	template<typename ...Args>
	static std::shared_ptr<T> GetInstance(Args&&... args) {
		if (!s_instance) {
			std::lock_guard<std::mutex> gLock(s_mutex);
			if (nullptr == s_instance) {
				s_instance = std::make_shared<T>(std::forward<Args>(args)...);
			}
		}
		return s_instance;
	}

	static void DesInstance() {
		if (s_instance) {
			s_instance.reset();
			s_instance = nullptr;
		}
	}


private:
	explicit Singleton();
	Singleton(const Singleton&) = delete;
	Singleton& operator=(const Singleton&) = delete;
	~Singleton();

private:
	static std::shared_ptr<T> s_instance;
	static std::mutex s_mutex;
};

template<typename T>
std::shared_ptr<T> Singleton<T>::s_instance = nullptr;

template<typename T>
std::mutex Singleton<T>::s_mutex;

