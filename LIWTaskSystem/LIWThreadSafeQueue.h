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
		private:
			typedef std::lock_guard<std::mutex> lock_guard;
			const std::chrono::milliseconds c_max_wait = std::chrono::milliseconds(1);
		public:
			LIWThreadSafeQueue() = default;
			LIWThreadSafeQueue(const LIWThreadSafeQueue&) = delete;
			LIWThreadSafeQueue& operator=(const LIWThreadSafeQueue&) = delete;

			/// <summary>
			/// Push a value into queue immediately. 
			/// </summary>
			/// <param name="val"> Value to enqueue. </param>
			/// <returns> Is operation successful. </returns>
			inline bool push_now(const T& val){
				lock_guard lock(__m_mtx_data);
				__m_queue.push(val);
				__m_cv_nonempty.notify_one();
				return true;
			}
			inline bool push_now(T&& val) {
				lock_guard lock(__m_mtx_data);
				__m_queue.emplace(val);
				__m_cv_nonempty.notify_one();
				return true;
			}

			/// <summary>
			/// Pop from queue immediately. 
			/// </summary>
			/// <param name="valOut"> Value dequeued. </param>
			/// <returns> Is operation successful. </returns>
			bool pop_now(T& valOut) {
				lock_guard lock(__m_mtx_data);
				if (__m_queue.empty()) {
					return false;
				}
				valOut = std::move(__m_queue.front());
				__m_queue.pop();
				return true;
			}

			/// <summary>
			/// Pop from queue when not empty. 
			/// </summary>
			/// <param name="valOut"> Value dequeued. </param>
			/// <returns> Is operation successful. Unsuccess means operation terminated. </returns>
			bool pop(T& valOut) {
				std::unique_lock<std::mutex> lock_empty(__m_mtx_data);
				while (__m_queue.empty() && __m_running) {
					__m_cv_nonempty.wait_for(lock_empty, c_max_wait);
				}
				if (__m_running) {
					valOut = std::move(__m_queue.front());
					__m_queue.pop();
					return true;
				}
				else {
					return false;
				}
			}

			/// <summary>
			/// Get a copy of the front of the queue. 
			/// </summary>
			/// <param name="valOut"> Copy of the front element. </param>
			/// <returns> Is operation successful. Unsuccess means queue empty. </returns>
			bool front(T& valOut) {
				lock_guard lk(__m_mtx_data);
				if (__m_queue.empty()) return false;
				valOut = __m_queue.front();
				return true;
			}
			/// <summary>
			/// Get a copy of the end of the queue. 
			/// </summary>
			/// <param name="valOut"> Copy of the last element. </param>
			/// <returns> Is operation successful. Unsuccess means queue empty. </returns>
			bool back(T& valOut) {
				lock_guard lk(__m_mtx_data);
				if (__m_queue.empty()) return false;
				valOut = __m_queue.back();
				return true;
			}
			/// <summary>
			/// Get size of queue. 
			/// </summary>
			/// <returns> Size of queue. </returns>
			inline typename size_type size() const { lock_guard lk(__m_mtx_data); return __m_queue.size(); }
			/// <summary>
			/// Get if queue is empty. 
			/// </summary>
			/// <returns> Is queue empty. </returns>
			inline bool empty() const { lock_guard lk(__m_mtx_data); return __m_queue.empty(); }

			/// <summary>
			/// Block the thread until queue is empty. 
			/// </summary>
			inline void block_till_empty() {
				while (!__m_queue.empty()) { std::this_thread::yield(); }
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
			mutable std::mutex __m_mtx_data;
			std::condition_variable __m_cv_nonempty;
			//std::condition_variable __m_cv_empty;
			bool __m_running = true;
		};
	}
}