#include <iostream>

#include <future>

void task()
{
	for (auto i = 0; i < 5; ++i)
	{
		std::cout << "task cycle " << std::to_string(i) << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds (500));
	}
}

int main(int argc, char *argv[]) {
	std::cout << std::numeric_limits<int16_t>::max() << std::endl;
	std::cout << std::numeric_limits<int32_t>::max() << std::endl;
	auto future = std::async(task);
	bool done = false;
	int i = 0;
	while (!done)
	{
		std::cout << "main cycle " << std::to_string(i++) << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds (500));
		if (future.wait_for(std::chrono::microseconds(0)) == std::future_status::ready)
			done = true;
	}
	std::cout << "done!" << std::endl;
}