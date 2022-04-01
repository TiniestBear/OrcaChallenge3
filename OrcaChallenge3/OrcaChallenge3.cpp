// OrcaChallenge3.cpp : Defines the entry point for the application.
//

#include <utility>
#include <vector>
#include <list>
#include <math.h>
#include <numeric>
#include <functional>
#include <fstream>
#include <string>
#include "json.hpp"
#include "OrcaChallenge3.h"

#define LOG_ENABLED false
#define LOGGER if (LOG_ENABLED)

using namespace std;
using json = nlohmann::json;
const static double TOLERANCE = 0.00001;

struct Line {
	pair<double, double> PointL, PointR;
	double ACoeff, BCoeff;
	Line(pair<double, double>& pointL, pair<double, double>& pointR) :
		PointL(pointL), PointR(pointR) {
		ACoeff = (PointR.second - PointL.second) / (PointR.first - PointL.first);
		BCoeff = PointL.second - ACoeff * PointL.first;
	
		LOGGER cout << "Line x1: " << this->PointL.first << " y1: " << this->PointL.second;
		LOGGER cout << " " << this->PointR.first << " y2: " << this->PointR.second << endl;
		LOGGER cout << "Line ACoeff: " << this->ACoeff << " BCoeff: " << this->BCoeff << endl;
	}
	//perpendicular to given and through point
	Line(Line& line, pair<double, double>& point): PointR(point) {
		if (line.ACoeff) {
			ACoeff = -line.ACoeff;
			BCoeff = PointR.second - ACoeff * PointR.first;
			PointL = IntersectPoint(line);
		}
		else {
			ACoeff = line.ACoeff;
			BCoeff = 0;
			PointL = make_pair(PointR.first, line.PointL.second);
		}
	}

	pair<double, double> IntersectPoint(Line& line) {
		double x = (this->BCoeff - line.BCoeff) / (line.ACoeff - this->ACoeff);
		double y = line.ACoeff * x + line.BCoeff;
		return make_pair(x, y);
	}

	bool Intersect(Line& line) {
		if (this->ACoeff == line.ACoeff) {
			LOGGER cout << "Line eq err" << endl;
			return false;
		}
		pair<double, double> intersectPoint = IntersectPoint(line);
		double& x = intersectPoint.first;

		pair<double, double> compare;

		if (this->PointL.first < this->PointR.first) {
			compare = { PointL.first, PointR.first };
		}
		else {
			compare = { PointR.first, PointL.first };
		}

		LOGGER cout << "X " << x << " bot " << compare.first << " top " << compare.second << endl;

		if (x < compare.first - TOLERANCE || x > compare.second + TOLERANCE) {
			LOGGER cout << "Line x scope err" << endl;
			return false;
		}

		double& y = intersectPoint.second;

		if (this->PointL.second < this->PointR.second) {
			compare = { PointL.second, PointR.second};
		}
		else {
			compare = { PointR.second, PointL.second };
		}

		LOGGER cout << "Y " << y << " bot " << compare.first << " top " << compare.second << endl;

		if (y < compare.first - TOLERANCE || y > compare.second + TOLERANCE) {
			LOGGER cout << "Line y scope err" << endl;
			return false;
		}

		return true;
	}

	double ShortestDist(Line& line) {

		return 0.0;
	}
};
struct TestLine : public Line {
	TestLine(pair<double, double>& pointL, pair<double, double>& pointR, vector<Line>::iterator& startGate, vector<Line>::iterator& stopGate) : 
		Line(pointL, pointR), StartGate(startGate), StopGate(stopGate) {}

	vector<Line>::iterator StartGate;
	vector<Line>::iterator StopGate;
};

double VectorLength(const Line& line) {
	return sqrt(pow((line.PointR.first - line.PointL.first), 2.0) + pow((line.PointR.second - line.PointL.second), 2.0));
}

void FindGoodThroughpoint(vector<Line>& pGates, list<TestLine>::iterator pLineIt, list<TestLine>& pLines, vector<Line>::iterator gateIt) {
	TestLine& line = *pLineIt;
	double d1, d2, distance = 0.0;
	vector<Line>::iterator splitGateIt = pGates.end();
	bool isFirst = false;
	for (auto it = gateIt; it != line.StopGate; ++it) {
		Line& gate = *it;
		Line perpendicularL = Line(line, gate.PointL);
		Line perpendicularR = Line(line, gate.PointR);
		d1 = VectorLength(perpendicularL);
		d2 = VectorLength(perpendicularR);
		double& pointDistance = d1 > d2 ? d2 : d1;
		if (distance > pointDistance) {
			continue;
		}

		distance = pointDistance;
		splitGateIt = it;
		isFirst = d1 > d2;
	}
 	if (splitGateIt != pGates.end()) {
		pair<double, double>& throughPoint = isFirst ? splitGateIt->PointR : splitGateIt->PointL;
		pLines.insert(pLineIt, TestLine(line.PointL, throughPoint, line.StartGate, splitGateIt));
		pLines.insert(pLineIt, TestLine(throughPoint, line.PointR, splitGateIt, line.StopGate));
		pLines.erase(pLineIt);
	}
}

int main()
{
	std::ifstream ifs("orcaData.json");
	std::string content((std::istreambuf_iterator<char>(ifs)),
		(std::istreambuf_iterator<char>()));

	json parsed = json::parse(content);
	//load gates automatically 
	vector<Line> gates;
	auto parsedGates = parsed["gates"];
	for (int index = 0; index < parsedGates.size();) {
		auto& lGate = parsedGates[index];
		pair<double, double> lPair = make_pair(lGate["x"], lGate["y"]);
		auto& rGate = parsedGates[++index];
		pair<double, double> rPair = make_pair(rGate["x"], rGate["y"]);
		gates.push_back(Line(lPair, rPair));
		++index;
	}

	list<TestLine> lines;
	auto parsedFrom = parsed["from"];
	pair<double, double> fromPair = make_pair(parsedFrom["x"], parsedFrom["y"]);
	auto parsedTo = parsed["to"];
	pair<double, double> toPair = make_pair(parsedTo["x"], parsedTo["y"]);
	lines.push_back(TestLine(fromPair, toPair, gates.begin(), gates.end()));
	list<TestLine> confirmedLines;
	bool allLinesOk;
	while (!lines.empty()) {
		allLinesOk = true;
		for (auto it = lines.begin(); it != lines.end(); it++) {
			LOGGER cout << "Line loops" << endl;
			auto gateIt = it->StartGate;
			for (; gateIt != it->StopGate; ++gateIt) {
				if (!gateIt->Intersect(*it)) {
					allLinesOk = false;
					break;
				}
			}
			if (!allLinesOk) {
				FindGoodThroughpoint(gates, it, lines, gateIt);
				break;
			} else {
				confirmedLines.splice(confirmedLines.begin(), lines, it);
				LOGGER cout << "Line splice" << endl;
				LOGGER cout << "Lines left " << lines.size() << endl;
				break;
			}
		}
	}

	if (LOG_ENABLED) {
		for (const auto& confirmedLine : confirmedLines) {
			cout << "Line start x: " << confirmedLine.PointL.first << " y: " << confirmedLine.PointL.second << endl;
			cout << "Line end x: " << confirmedLine.PointR.first << " y: " << confirmedLine.PointR.second << endl;
		}
	}

	auto distance = accumulate(confirmedLines.begin(), confirmedLines.end(), 0.0, [](double& init, Line& line) {
		return init + VectorLength(line);
	});
	cout << "Total distance " << distance << endl;
	return 0;
}
