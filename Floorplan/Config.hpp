//
// @author   liyan
// @contact  lyan_dut@outlook.com
//
#pragma once

#ifdef NDEBUG
#define debug_run(x)
#else
#define debug_run(x) x
#endif // !NDEBUG

#include <iostream>
#include <vector>
#include <utility>
#include <string>
#include <random>
#include <unordered_map>

static constexpr int INF = 0x3f3f3f3f;

static std::vector<std::pair<std::string, std::string>> ins_list{
	{"MCNC", "apte"},{"MCNC", "xerox"},{"MCNC", "hp"},{"MCNC", "ami33"},{"MCNC", "ami49"},
	{"GSRC", "n10"},{"GSRC", "n30"},{"GSRC", "n50"},{"GSRC", "n100"},{"GSRC", "n200"},{"GSRC", "n300"}
};

static std::unordered_map<std::string, std::pair<int, int>> obj_map{
	{"apte", {47050000, 246000}}, {"xerox", {20340000, 379600}}, {"hp", {9460000, 149800}},
	{"ami33", {1220000, 59500}}, {"ami49", {37820000, 667000}},
	{"n10", {225242, 25788}}, {"n30", {216051, 79740}}, {"n50", {204295, 124326}},
	{"n100", {187880, 206269}}, {"n200", {183931, 389272}}, {"n300", {288702, 587739}}
};

/// �㷨��������
struct Config {
	unsigned int random_seed = std::random_device{}(); // ������ӣ�release�汾ʹ��`random_device{}();`

	double alpha = 0.99, beta = 0.01;      // ����������߳��Ż�Ȩ��
	double lb_scale = 0.8, ub_scale = 1.2; // ���Ʒ�֧��Ŀ�������

	int ub_time = 3600; // ASA��ʱʱ��
	int ub_iter = 8192; // RLS����������/BS��������

	int dimension = 5;  // QAP������ά��. e.g.dimension=5�򻮷�25�����飬����25��block�����Ͻ�

	enum class LevelCandidateWidth {
		CombRotate,            // [deprecated] ������ϼ���ת���������
		CombShort,             // [deprecated] ���Ƕ̱ߵ����
		Interval,              // �Լ���Ⱦ໮�ֿ������
		Sqrt                   // ����ƽ�������Ƴ����
	} level_asa_cw = LevelCandidateWidth::Interval;

	enum class LevelFloorplanPacker {
		RandomLocalSearch,
		BeamSearch,        // �Ż��汾 ==> ��Ҫ�Ż��������/�²������ƣ�
		DecisionSearch     // �ж��汾 ==> ��Ҫ�Ż��߳�
	} level_asa_fbp = LevelFloorplanPacker::DecisionSearch;

	enum class LevelWireLength {
		BlockOnly,         // ����block�ڲ������߳�
		BlockAndTerminal   // [deprecated] ����block��terminal�����߳�
	} level_fbp_wl = LevelWireLength::BlockOnly;

	enum class LevelObjDist { // EDAthon2020P4Ŀ�꺯����dist���㷽��
		SqrHpwlDist,          // ����������ܳ���ƽ��
		SqrEuclideanDist,     // ���ζ�֮��ŷʽƽ������
		SqrManhattanDist      // ���ζ�֮��������ƽ������
	} level_fbp_dist = LevelObjDist::SqrHpwlDist;

	enum class LevelQAPCluster {
		On,
		Off // ��ʹ��QAP
	} level_rls_qapc = LevelQAPCluster::Off;

	enum class LevelGroupSearch {
		Off,             // �����飬LevelQAPCluster::Off <=> LevelGroupSearch::Off
		NeighborNone,    // ��ǰ����
		NeighborAll,     // ��ǰ�����ȫ���ھӷ���
		NeighborPartial  // ��ǰ����������ھӷ��飬[todo]���ǰ��ٷֱ�ѡȡһ���־���?
	} level_rls_gs = LevelGroupSearch::Off;

	enum class LevelGraphConnect {
		Direct,  // ������ֱ���ıߣ���ֱ���ı�Ȩ�ض���Ϊ 0
		Indirect // ���Ƿ�ֱ���ıߣ���ֱ���ı�Ȩ�ض���Ϊ `1/���·����`
	} level_qapc_gc = LevelGraphConnect::Direct;

	enum class LevelFlow {
		Kway,
		Recursive
	} level_qapc_flow = LevelFlow::Kway;

	enum class LevelDist {
		EuclideanDist,   // ŷ����þ���
		ManhattanDist,   // �����پ���
		ChebyshevDist    // �б�ѩ�����
	} level_qapc_dist = LevelDist::ManhattanDist;
} cfg;

std::ostream& operator<<(std::ostream& os, const Config& cfg) {
	switch (cfg.level_asa_fbp) {
	case Config::LevelFloorplanPacker::RandomLocalSearch: os << "RandomLocalSearch,"; break;
	case Config::LevelFloorplanPacker::BeamSearch: os << "BeamSearch,"; break;
	case Config::LevelFloorplanPacker::DecisionSearch: os << "DecisionSearch,"; break;
	default: break;
	}

	switch (cfg.level_fbp_wl) {
	case Config::LevelWireLength::BlockOnly: os << "BlockOnly,"; break;
	case Config::LevelWireLength::BlockAndTerminal: os << "BlockAndTerminal,"; break;
	default: break;
	}

	switch (cfg.level_fbp_dist) {
	case Config::LevelObjDist::SqrHpwlDist: os << "SqrHpwlDist,"; break;
	case Config::LevelObjDist::SqrEuclideanDist: os << "SqrEuclideanDist,"; break;
	case Config::LevelObjDist::SqrManhattanDist: os << "SqrManhattanDist,"; break;
	default: break;
	}

	switch (cfg.level_rls_qapc) {
	case Config::LevelQAPCluster::On: os << "On,"; break;
	case Config::LevelQAPCluster::Off: os << "Off,"; break;
	default: break;
	}

	switch (cfg.level_rls_gs) {
	case Config::LevelGroupSearch::Off: os << "Off,"; break;
	case Config::LevelGroupSearch::NeighborNone: os << "NeighborNone,"; break;
	case Config::LevelGroupSearch::NeighborAll: os << "NeighborAll,"; break;
	case Config::LevelGroupSearch::NeighborPartial: os << "NeighborPartial,"; break;
	default: break;
	}

	switch (cfg.level_qapc_gc) {
	case Config::LevelGraphConnect::Direct: os << "Direct,"; break;
	case Config::LevelGraphConnect::Indirect: os << "Indirect,"; break;
	default: break;
	}

	switch (cfg.level_qapc_flow) {
	case Config::LevelFlow::Kway: os << "Kway,"; break;
	case Config::LevelFlow::Recursive:os << "Recursive,"; break;
	default: break;
	}

	switch (cfg.level_qapc_dist) {
	case Config::LevelDist::EuclideanDist: os << "EuclideanDist"; break;
	case Config::LevelDist::ChebyshevDist: os << "ChebyshevDist"; break;
	case Config::LevelDist::ManhattanDist: os << "ManhattanDist"; break;
	default: break;
	}

	return os;
}
