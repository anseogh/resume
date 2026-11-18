#pragma once

#include "CoreMinimal.h"

// All player-facing strings live here so they can be localized in one place.
// This file must stay UTF-8 with BOM so MSVC reads the Korean literals correctly.
namespace OCText
{
	inline constexpr const TCHAR* Wasted = TEXT("사망");
	inline constexpr const TCHAR* WastedSub = TEXT("병원에서 깨어납니다  (-$500)");
	inline constexpr const TCHAR* MissionPassed = TEXT("미션 완료");
	inline constexpr const TCHAR* MissionFailed = TEXT("미션 실패");
	inline constexpr const TCHAR* FailedDied = TEXT("당신은 죽었습니다");
	inline constexpr const TCHAR* LostCops = TEXT("경찰을 따돌렸다");
	inline constexpr const TCHAR* WantedUp = TEXT("수배 레벨 상승!");
	inline constexpr const TCHAR* EnterVehicle = TEXT("[F] 차량 탑승");
	inline constexpr const TCHAR* ExitVehicle = TEXT("[F] 하차");
	inline constexpr const TCHAR* VehicleOnFire = TEXT("차량에 불이 붙었다! 빨리 내려!");
	inline constexpr const TCHAR* GoToMissionStart = TEXT("노란 표시로 가서 다음 미션을 시작하세요");
	inline constexpr const TCHAR* StoryComplete = TEXT("스토리 완료 - 도시는 이제 당신의 것");
	inline constexpr const TCHAR* ObjectiveComplete = TEXT("목표 달성");
	inline constexpr const TCHAR* StoryMissing = TEXT("Content/Story/Story.json 을 찾을 수 없습니다 - 자유 모드");
	inline constexpr const TCHAR* Seconds = TEXT("초");
	inline constexpr const TCHAR* Meters = TEXT("m");
	inline constexpr const TCHAR* Kmh = TEXT("km/h");

	inline constexpr const TCHAR* ObjGoTo = TEXT("목적지로 이동하라");
	inline constexpr const TCHAR* ObjEnterVehicle = TEXT("차량에 탑승하라");
	inline constexpr const TCHAR* ObjDriveTo = TEXT("차를 몰고 목적지로 가라");
	inline constexpr const TCHAR* ObjEliminate = TEXT("적을 모두 처치하라");
	inline constexpr const TCHAR* ObjLoseWanted = TEXT("경찰을 따돌려라");
	inline constexpr const TCHAR* ObjSurvive = TEXT("버텨라");
	inline constexpr const TCHAR* ObjGetWanted = TEXT("수배 레벨을 올려라");
}
