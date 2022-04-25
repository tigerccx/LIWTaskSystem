#pragma once
#include <functional>

namespace LIW {
	typedef std::function<void* (void*)> LIWThreadTask;
	typedef std::function<void* (void*)> LIWThreadTaskCallback;

	class LIWITask {
	public:
		LIWITask() = default;
		virtual ~LIWITask() = default;

		//virtual void BlockForDependencies() = 0;
		virtual void Execute(void*) = 0;
		//virtual void SignalDependents() = 0;
	};
}