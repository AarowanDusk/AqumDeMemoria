#pragma once

//HelperMacros
#if 0
float MacroDuration = 10.f;
#define SLOG(x) GEngine->AddOnScreenDebugMessage(-1, MacroDuration ? MacroDuration: -1.f, FColor::MakeRandomColor(), x);
#define POINT(x, c) DrawDebugPoint(GetWorld(), x, 10, c, !MacroDuration, MacroDuration);
#define LINE(x1, x2, c) DrawDebugLine(GetWorld(), x1, x2, c, !MacroDuration, MacroDuration);
#define CAPSULE(x, c) DrawDebugCapsule(GetWorld(), x, CapHH(), CapR(), FQuat::Identity, c, !MacroDuration, MacroDuration);
#else
#define SLOG(x)
#define POINT(x, c)
#define LINE(x1, x2, c)
#define CAPSULE(x, c)
#endif	

namespace Debug
{

	static void Print(const FString& Msg, const FColor& Color = FColor::MakeRandomColor(), int32 InKey = -1)
	{

		if (GEngine)
		{

			GEngine->AddOnScreenDebugMessage(InKey, 6.f, Color, Msg);
		
		}

		UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);

	}

}