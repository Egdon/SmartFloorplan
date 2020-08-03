//
// @author   liyan
// @contact  lyan_dut@outlook.com
//
#pragma once

#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace utils {
	
	using namespace std;

	class Date {
	public:
		// ���ر�ʾ���ڸ�ʽ���ַ�������-��-��
		static const string to_short_str() {
			ostringstream os;
			time_t now = time(0);
			os << put_time(localtime(&now), "%y-%m-%e");
			return os.str();
		}
		// ���ر�ʾ���ڸ�ʽ���ַ�������������
		static const string to_detail_str() {
			ostringstream os;
			time_t now = time(0);
			//os << put_time(localtime(&now), "%y-%m-%e-%H_%M_%S");
			os << put_time(localtime(&now), "%Y%m%d%H%M%S");
			return os.str();
		}
	};

	// �����ļ��е�n�� 
	static void skip(FILE *file, int line_num) {
		char linebuf[1000];
		for (int i = 0; i < line_num; ++i) {
			fgets(linebuf, sizeof(linebuf), file); // skip
		}
	}
}
