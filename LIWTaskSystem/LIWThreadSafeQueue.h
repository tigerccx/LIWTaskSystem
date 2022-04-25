#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

namespace LIW {
	namespace Util {
		template<class T>
		class LIWThreadSafeQueue {
		public:
			typedef typename std::queue<T>::size_type size_type;

			LIWThreadSafeQueue() = default;
			LIWThreadSafeQueue(const LIWThreadSafeQueue&) = delete;
			LIWThreadSafeQueue& operator=(const LIWThreadSafeQueue&) = delete;

			/// <summary>
			/// Push a value into queue. 
			/// </summary>
			/// <param name="val"> Value to enqueue. </param>
			inline void push(const T& val){
				std::lock_guard<std::mutex> lock(__m_mtx_data);
				__m_queue.push(val);
				__m_cv_nonempty.notify_one();
			}
			inline void push(T&& val) {
				std::lock_guard<std::mutex> lock(__m_mtx_data);
				__m_queue.emplace(val);
				__m_cv_nonempty.notify_one();
			}

			/// <summary>
			/// Pop from queue immediately. 
			/// </summary>
			/// <param name="valOut"> Value dequeued. </param>
			/// <returns> Size of queue after poping. If -1, meaning the queue is empty. </returns>
			int pop_now(T& valOut) {
				std::lock_guard<std::mutex> lock(__m_mtx_data);
				if (__m_queue.empty()) {
					return -1;
				}
				valOut = std::move(__m_queue.front());
				__m_queue.pop();
				return (int)__m_queue.size();
			}

			/// <summary>
			/// Pop from queue when not empty. 
			/// </summary>
			/// <param name="valOut"> Value dequeued. </param>
			/// <returns> Size of queue after poping. If -1, meaning the queue is finishing up. </returns>
			int pop(T& valOut) {
				std::unique_lock<std::mutex> lock_empty(__m_mtx_data);
				while (__m_queue.empty() && __m_running) {
					__m_cv_nonempty.wait(lock_empty);
				}
				if (__m_running) {
					valOut = std::move(__m_queue.front());
					__m_queue.pop();
					return (int)__m_queue.size();
				}
				else
				{
					return -1;
				}
			}

			/// <summary>
			/// Get a copy of the front of the queue. 
			/// </summary>
			/// <returns> Copy of the front element. </returns>
			inline T front() { return __m_queue.front(); }
			/// <summary>
			/// Get a copy of the end of the queue. 
			/// </summary>
			/// <returns> Copy of the last element. </returns>
			inline T back() { return __m_queue.back(); }
			/// <summary>
			/// Get size of queue. 
			/// </summary>
			/// <returns> Size of queue. </returns>
			inline typename size_type size() const { return __m_queue.size(); }
			/// <summary>
			/// Get if queue is empty. 
			/// </summary>
			/// <returns> Is queue empty. </returns>
			inline bool empty() const { return __m_queue.empty(); }

			/// <summary>
			/// Block the thread until queue is empty. 
			/// </summary>
			inline void block_till_empty() {
				while (!__m_queue.empty()) { std::this_thread::yield(); }
				//std::unique_lock<std::mutex> lock_nonempty(__m_mtx_data);
				//__m_cv_empty.wait(lock_nonempty, [&]() {return __m_queue.empty(); });
			}
			/// <summary>
			/// Notify all pop() calls to stop blocking and exit. 
			/// </summary>
			inline void notify_stop() {
				__m_running = false;
				__m_cv_nonempty.notify_all();
			}

		protected:
			std::queue<T> __m_queue;
		private:
			std::mutex __m_mtx_data;
			std::condition_variable __m_cv_nonempty;
			//std::condition_variable __m_cv_empty;
			bool __m_running = true;
		};
	}
}