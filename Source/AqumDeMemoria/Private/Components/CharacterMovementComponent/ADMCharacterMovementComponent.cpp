// Copyright, Bhargav Jt. Gogoi (Aarowan Vespera)

#include "Components/CharacterMovementComponent/ADMCharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Characters/Player/ADMCharacter.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/KismetMathLibrary.h"
#include "Debug/Debug.h"

void UADMCharacterMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TickCount++;
	if (IsNetMode(NM_Client))
	{

		GEngine->AddOnScreenDebugMessage(2, 100.f, FColor::Yellow, FString::Printf(TEXT("Correction: %.2f"), 100.f * (float)CorrectionCount / (float)TickCount));
		GEngine->AddOnScreenDebugMessage(9, 100.f, FColor::Yellow, FString::Printf(TEXT("Bitrate: %.3f"), (float)TotalBitsSent / GetWorld()->GetTimeSeconds() / 1000.f));
	
	}
	else
	{

		GEngine->AddOnScreenDebugMessage(3, 100.f, FColor::Yellow, FString::Printf(TEXT("Location Error: %.4f cm/s"), 100.f * AccumulatedClientLocationError / GetWorld()->GetTimeSeconds()));
	
	}

}

UADMCharacterMovementComponent::UADMCharacterMovementComponent()
{

	NavAgentProps.bCanCrouch = true;
	ADMServerMoveBitWriter.SetAllowResize(true);

}

#pragma region ReplicationLogic

bool UADMCharacterMovementComponent::FSavedMove_ADM::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{

	FSavedMove_ADM* NewADMMove = static_cast<FSavedMove_ADM*>(NewMove.Get());

	if (Saved_bWallRunIsRight != NewADMMove->Saved_bWallRunIsRight)
	{
		return false;
	}

	return CanCombineWith(NewMove, InCharacter, MaxDelta);

}

void UADMCharacterMovementComponent::FSavedMove_ADM::Clear()
{

	FSavedMove_Character::Clear();

	Saved_bADMPressedJump = 0;

	Saved_bHadAnimRootMotion = 0;
	Saved_bTransitionFinished = 0;

	Saved_bWallRunIsRight = 0;

}

uint8 UADMCharacterMovementComponent::FSavedMove_ADM::GetCompressedFlags() const
{

	uint8 CompressedFlags = Super::GetCompressedFlags();

	if (Saved_bADMPressedJump) CompressedFlags |= FLAG_JumpPressed;

	return CompressedFlags;

}

void UADMCharacterMovementComponent::FSavedMove_ADM::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{

	Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);

	UADMCharacterMovementComponent* MovementComponent = Cast<UADMCharacterMovementComponent>(Character->GetCharacterMovement());
	if (MovementComponent)
	{

		Saved_bADMPressedJump = MovementComponent->ADMCharacterOwner->bADMPressedJump;

		Saved_bHadAnimRootMotion = MovementComponent->Safe_bHadAnimRootMotion;
		Saved_bTransitionFinished = MovementComponent->Safe_bTransitionFinished;

		Saved_bWantsToGlide = MovementComponent->Safe_bWantsToGlide;

		Saved_bWallRunIsRight = MovementComponent->Safe_bWallRunIsRight;

	}

}

void UADMCharacterMovementComponent::FSavedMove_ADM::PrepMoveFor(ACharacter* Character)
{

	Super::PrepMoveFor(Character);

	UADMCharacterMovementComponent* MovementComponent = Cast<UADMCharacterMovementComponent>(Character->GetCharacterMovement());
	if (MovementComponent)
	{

		MovementComponent->ADMCharacterOwner->bADMPressedJump = Saved_bADMPressedJump;

		MovementComponent->Safe_bHadAnimRootMotion = Saved_bHadAnimRootMotion;
		MovementComponent->Safe_bTransitionFinished = Saved_bTransitionFinished;

		MovementComponent->Safe_bWantsToGlide = Saved_bWantsToGlide;

		MovementComponent->Safe_bWallRunIsRight = Saved_bWallRunIsRight;

	}

}

UADMCharacterMovementComponent::FNetworkPredictionData_Client_ADM::FNetworkPredictionData_Client_ADM(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{



}

FSavedMovePtr UADMCharacterMovementComponent::FNetworkPredictionData_Client_ADM::AllocateNewMove()
{

	return FSavedMovePtr(new FSavedMove_ADM());

}

FNetworkPredictionData_Client* UADMCharacterMovementComponent::GetPredictionData_Client() const
{

	check(PawnOwner != NULL);

	if (ClientPredictionData == nullptr)
	{

		UADMCharacterMovementComponent* MutableThis = const_cast<UADMCharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_ADM(*this);
		MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.f;
		MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.f;

	}

	return ClientPredictionData;

}

void UADMCharacterMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{

	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UADMCharacterMovementComponent, Proxy_bTallMantle, COND_SkipOwner)
	DOREPLIFETIME_CONDITION(UADMCharacterMovementComponent, Proxy_bShortMantle, COND_SkipOwner)

}

float UADMCharacterMovementComponent::GetMaxSpeed() const
{

	if (MovementMode != MOVE_Custom) return Super::GetMaxSpeed();

	switch (CustomMovementMode)
	{

		case CMOVE_Hang:
			return 0.f;

		case CMOVE_Glide:
			return GlideMaxWalkSpeed;

		case CMOVE_WallRun:
			return MaxWallRunSpeed;

		default:
			UE_LOG(LogTemp, Fatal, TEXT("Invalid Movement Mode"));
			return -1.f;

	}

}

float UADMCharacterMovementComponent::GetMaxBrakingDeceleration() const
{

	if (MovementMode != MOVE_Custom) return Super::GetMaxBrakingDeceleration();

	switch (CustomMovementMode)
	{

		case CMOVE_Hang:
			return 0.f;

		case CMOVE_Glide:
			return 350.f;

		case CMOVE_WallRun:
			return 0.f;

		default:
			UE_LOG(LogTemp, Fatal, TEXT("Invalid Movement Mode"));
			return -1.f;

	}

}

bool UADMCharacterMovementComponent::CanAttemptJump() const
{

	UE_LOG(LogTemp, Warning, TEXT("%s"), IsWallRunning() ? TEXT("True"):TEXT("False"));
	SLOG("MOVE_Falling Converted to, Starting new physics");

	return Super::CanAttemptJump() || IsWallRunning() || IsHanging();

}

bool UADMCharacterMovementComponent::DoJump(bool bReplayingMoves)
{

	bool bWasOnWall = IsHanging();
	bool bWasWallRunning = IsWallRunning();

	if (Super::DoJump(bReplayingMoves))
	{

		if (bWasOnWall)
		{

			if (!bReplayingMoves)
			{

				CharacterOwner->PlayAnimMontage(WallJumpMontage);
				Velocity += FVector::UpVector * WallJumpForce * 0.5f;
				Velocity += Acceleration.GetSafeNormal2D() * WallJumpForce * 0.5f;

			}

		}

		if (bWasWallRunning)
		{

			FVector CastDelta = UpdatedComponent->GetRightVector() * CapR() * 2;
			FVector Start = UpdatedComponent->GetComponentLocation();
			FVector End = Safe_bWallRunIsRight ? Start + CastDelta : Start - CastDelta;
			auto Params = ADMCharacterOwner->GetIgnoreCharacterParams();
			FHitResult WallHit;
			GetWorld()->LineTraceSingleByProfile(WallHit, Start, End, "BlockAll", Params);
			Velocity += WallHit.Normal * WallJumpOffForce;

		}

		return true;

	}

	return false;

}

void UADMCharacterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{

	//TryMantle
	if (ADMCharacterOwner->bADMPressedJump)
	{

		if (TryMantle())
		{

			//SLOG("Trying Mantle")
			ADMCharacterOwner->StopJumping();

		}
		else if (TryHang())
		{

			//SLOG("Trying Hang")
			ADMCharacterOwner->StopJumping();

		}
		else if (IsFalling())
		{

			if (CanGlide())
			{

				//SLOG("Trying Glide")

				ADMCharacterOwner->StopJumping();

				SetMovementMode(MOVE_Custom, CMOVE_Glide);
				if (!CharacterOwner->HasAuthority()) Server_EnterGlide();

			}

			Safe_bWantsToGlide = false;

		}
		else
		{

			SLOG("Failed, Reverting to jump")
			ADMCharacterOwner->bADMPressedJump = false;
			CharacterOwner->bPressedJump = true;
			CharacterOwner->CheckJumpInput(DeltaSeconds);
			bOrientRotationToMovement = true;

			SetMovementMode(MOVE_Falling);

		}

	}

	//TransitionMontage
	if (Safe_bTransitionFinished)
	{

		SLOG("Transiton Finished")
		UE_LOG(LogTemp, Warning, TEXT("FINISHED RM"))

		if (IsValid(TransitionQueueMontage))
		{

			SetMovementMode(MOVE_Flying);
			CharacterOwner->PlayAnimMontage(TransitionQueueMontage, TransitionQueuedMontageSpeed);
			TransitionQueuedMontageSpeed = 0.f;
			TransitionQueueMontage = nullptr;

		}
		else
		{

			SetMovementMode(MOVE_Walking);

		}

		Safe_bTransitionFinished = false;

	}


	//IfGlidingEnds
	
	//WallRun
	if (IsFalling())
	{

		//SLOG("Trying Wallrun");

		UE_LOG(LogTemp, Warning, TEXT("Is it true? %s"), TryWallRun() ? TEXT("true") : TEXT("false"));

	}

	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);

}

void UADMCharacterMovementComponent::UpdateCharacterStateAfterMovement(float DeltaSeconds)
{

	Super::UpdateCharacterStateAfterMovement(DeltaSeconds);

	if (!HasAnimRootMotion() && Safe_bHadAnimRootMotion && IsMovementMode(MOVE_Flying))
	{

		UE_LOG(LogTemp, Warning, TEXT("Endng Anim Rootmotio"))
		SetMovementMode(MOVE_Walking);

	}

	if (GetRootMotionSourceByID(TransitionRMS_ID) && GetRootMotionSourceByID(TransitionRMS_ID)->Status.HasFlag(ERootMotionSourceStatusFlags::Finished))
	{

		RemoveRootMotionSourceByID(TransitionRMS_ID);
		Safe_bTransitionFinished = true;

	}

	Safe_bHadAnimRootMotion = HasAnimRootMotion();

}

void UADMCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{

	Super::UpdateFromCompressedFlags(Flags);

}

void UADMCharacterMovementComponent::OnClientCorrectionReceived(FNetworkPredictionData_Client_Character& ClientData, float TimeStamp, FVector NewLocation, FVector NewVelocity, UPrimitiveComponent* NewBase, FName NewBaseBoneName, bool bHasBase, bool bBaseRelativePosition, uint8 ServerMovementMode, FVector ServerGravityDirection)
{

	Super::OnClientCorrectionReceived(ClientData, TimeStamp, NewLocation, NewVelocity, NewBase, NewBaseBoneName, bHasBase, bBaseRelativePosition, ServerMovementMode, ServerGravityDirection);

	CorrectionCount++;

}

void UADMCharacterMovementComponent::InitializeComponent()
{

	Super::InitializeComponent();

	ADMCharacterOwner = Cast<AADMCharacter>(GetOwner());

}

bool UADMCharacterMovementComponent::ServerCheckClientError(float ClientTimeStamp, float DeltaTime, const FVector& Accel, const FVector& ClientWorldLocation, const FVector& RelativeClientLocation, UPrimitiveComponent* ClientMovementBase, FName ClientBaseBoneName, uint8 ClientMovementMode)
{
	
	if (GetCurrentNetworkMoveData()->NetworkMoveType == FCharacterNetworkMoveData::ENetworkMoveType::NewMove)
	{

		float LocationError = FVector::Dist(UpdatedComponent->GetComponentLocation(), ClientWorldLocation);
		GEngine->AddOnScreenDebugMessage(6, 100.f, FColor::Yellow, FString::Printf(TEXT("Loc: %s"), *ClientWorldLocation.ToString()));
		AccumulatedClientLocationError += LocationError * DeltaTime;

	}

	return Super::ServerCheckClientError(ClientTimeStamp, DeltaTime, Accel, ClientWorldLocation, RelativeClientLocation, ClientMovementBase, ClientBaseBoneName, ClientMovementMode);

}

void UADMCharacterMovementComponent::CallServerMovePacked(const FSavedMove_Character* NewMove, const FSavedMove_Character* PendingMove, const FSavedMove_Character* OldMove)
{

	// Get storage container we'll be using and fill it with movement data
	FCharacterNetworkMoveDataContainer& MoveDataContainer = GetNetworkMoveDataContainer();
	MoveDataContainer.ClientFillNetworkMoveData(NewMove, PendingMove, OldMove);

	// Reset bit writer without affecting allocations
	FBitWriterMark BitWriterReset;
	BitWriterReset.Pop(ADMServerMoveBitWriter);

	// 'static' to avoid reallocation each invocation
	static FCharacterServerMovePackedBits PackedBits;
	UNetConnection* NetConnection = CharacterOwner->GetNetConnection();

	{

		// Extract the net package map used for serializing object references.
		ADMServerMoveBitWriter.PackageMap = NetConnection ? ToRawPtr(NetConnection->PackageMap) : nullptr;

	}

	if (ADMServerMoveBitWriter.PackageMap == nullptr)
	{

		UE_LOG(LogNetPlayerMovement, Error, TEXT("CallServerMovePacked: Failed to find a NetConnection/PackageMap for data serialization!"));
		return;

	}

	// Serialize move struct into a bit stream
	if (!MoveDataContainer.Serialize(*this, ADMServerMoveBitWriter, ADMServerMoveBitWriter.PackageMap) || ADMServerMoveBitWriter.IsError())
	{

		UE_LOG(LogNetPlayerMovement, Error, TEXT("CallServerMovePacked: Failed to serialize out movement data!"));
		return;

	}

	// Copy bits to our struct that we can NetSerialize to the server.
	PackedBits.DataBits.SetNumUninitialized(ADMServerMoveBitWriter.GetNumBits());

	check(PackedBits.DataBits.Num() >= ADMServerMoveBitWriter.GetNumBits());
	FMemory::Memcpy(PackedBits.DataBits.GetData(), ADMServerMoveBitWriter.GetData(), ADMServerMoveBitWriter.GetNumBytes());

	TotalBitsSent += PackedBits.DataBits.Num();

	// Send bits to server!
	ServerMovePacked_ClientSend(PackedBits);

	MarkForClientCameraUpdate();

}

void UADMCharacterMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{

	Super::PhysCustom(deltaTime, Iterations);

	switch (CustomMovementMode)
	{

		case CMOVE_Hang:
			break;

		case CMOVE_Glide:
			PhysGlide(deltaTime, Iterations);
			break;

		case CMOVE_WallRun:
			PhysWallRun(deltaTime, Iterations);
			break;

		default:
			UE_LOG(LogTemp, Fatal, TEXT("Invalid Movement Mode"));

	}

}

void UADMCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{

	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == CMOVE_Glide) ExitGlide();

	if (IsCustomMovementMode(CMOVE_Glide)) 
	{

		EnterGlide(PreviousMovementMode, (ECustomMovementMode)PreviousCustomMode);
	
	}

	if (IsWallRunning() && GetOwnerRole() == ROLE_SimulatedProxy)
	{

		FVector Start = UpdatedComponent->GetComponentLocation();
		FVector End = UpdatedComponent->GetRightVector() * CapR() *2;
		auto Params = ADMCharacterOwner->GetIgnoreCharacterParams();
		FHitResult WallHit;
		Safe_bWallRunIsRight = GetWorld()->LineTraceSingleByProfile(WallHit, Start, End, "BlockAll", Params);

	}

}

#pragma endregion

#pragma region MantleFunctions

bool UADMCharacterMovementComponent::TryMantle()
{

	if (!IsMovementMode(MOVE_Walking) && !IsMovementMode(MOVE_Flying)) return false;

	//HelperVariables
	FVector BaseLoc = UpdatedComponent->GetComponentLocation() + FVector::DownVector * CapHH();
	FVector Fwd = UpdatedComponent->GetForwardVector().GetSafeNormal2D();
	auto Params = ADMCharacterOwner->GetIgnoreCharacterParams();
	float MaxHeight = CapHH() * 2 + MantleReachHeight;
	float CosMMWSA = FMath::Cos(FMath::DegreesToRadians(MantoleMinWallSteepnessAngle));
	float CosMMSA = FMath::Cos(FMath::DegreesToRadians(MantoleMaxSurfaceAngle));
	float CosMMAA = FMath::Cos(FMath::DegreesToRadians(MantleMaxAlignmentAngle));
	
	SLOG("Starting Mantle Attempt")

	//CheckFrontFace
	FHitResult FrontHit;
	float CheckDistance = FMath::Clamp(Velocity | Fwd, CapR() + 30, MantleMaxDistance);
	FVector FrontStart = BaseLoc + FVector::UpVector * (MaxStepHeight - 1);
	for(int i=0; i<6; i++) 
	{

		LINE(FrontStart, FrontStart + Fwd * CheckDistance, FColor::Red)
		if (GetWorld()->LineTraceSingleByProfile(FrontHit, FrontStart, FrontStart + Fwd * CheckDistance, "BlockAll", Params)) break;
		FrontStart += FVector::UpVector * (2.f * CapHH() - (MaxStepHeight - 1)) / 5;

	}
	if (!FrontHit.IsValidBlockingHit()) return false;
	float CosWallSteepnessAngle = FrontHit.Normal | FVector::UpVector;
	if (FMath::Abs(CosWallSteepnessAngle) > CosMMWSA || (Fwd | -FrontHit.Normal) < CosMMAA) return false;
	POINT(FrontHit.Location, FColor::Red)

	//CheckHeight
	TArray<FHitResult> HeightHits;
	FHitResult SurfaceHit;
	FVector WallUp = FVector::VectorPlaneProject(FVector::UpVector, FrontHit.Normal).GetSafeNormal();
	float WallCos = FVector::UpVector | FrontHit.Normal;
	float WallSin = FMath::Sqrt(1 - WallCos * WallCos);
	FVector TraceStart = FrontHit.Location + Fwd + WallUp * (MaxHeight - (MaxStepHeight - 1)) / WallSin;
	LINE(TraceStart, FrontHit.Location + Fwd, FColor::Orange);
	if(!GetWorld()->LineTraceMultiByProfile(HeightHits, TraceStart, FrontHit.Location + Fwd, "BlockAll", Params)) return false;
	for (const FHitResult& Hit : HeightHits)
	{
		
		if (Hit.IsValidBlockingHit())
		{
			
			SurfaceHit = Hit;
			break;

		}

	}
	if (!SurfaceHit.IsValidBlockingHit() || (SurfaceHit.Normal | FVector::UpVector) < CosMMSA) return false;
	float Height = (SurfaceHit.Location - BaseLoc) | FVector::UpVector;
	SLOG(FString::Printf(TEXT("Heght: %f"), Height));
	POINT(SurfaceHit.Location, FColor::Blue)
	if (Height > MaxHeight) return false;

	//CheckClearence
	float SurfaceCos = FVector::UpVector | SurfaceHit.Normal;
	float SurfaceSin = FMath::Sqrt(1 - SurfaceCos * SurfaceCos);
	FVector ClearCapLoc = SurfaceHit.Location + Fwd * CapR() + FVector::UpVector * (CapHH() + 1 + CapR() * 2 * SurfaceSin);
	FCollisionShape CapShape = FCollisionShape::MakeCapsule(CapR(), CapHH());
	if (GetWorld()->OverlapAnyTestByProfile(ClearCapLoc, FQuat::Identity, "BlockAll", CapShape, Params))
	{
		
		CAPSULE(ClearCapLoc, FColor::Red)
		return false;

	}
	else
	{
		
		CAPSULE(ClearCapLoc, FColor::Green)

	}

	SLOG("Can Mantle")

	//MantleSelection
	FVector TallMantleTarget = GetMantelStartLocation(FrontHit, SurfaceHit, true);
	FVector ShortMantleTarget = GetMantelStartLocation(FrontHit, SurfaceHit, false);
	bool bTallMantle = false;
	if (IsMovementMode(MOVE_Falling) && Height > CapHH() * 2)
	{
		
		bTallMantle = true;

	}
	else if (IsMovementMode(MOVE_Falling) && (Velocity | FVector::UpVector) < 0)
	{
		
		if (!GetWorld()->OverlapAnyTestByProfile(TallMantleTarget, FQuat::Identity, "BlockALL", CapShape, Params))
		{
			
			bTallMantle = true;

		}

	}
	FVector TransitionTarget = bTallMantle ? TallMantleTarget : ShortMantleTarget;
	CAPSULE(TransitionTarget, FColor::Yellow)
	CAPSULE(UpdatedComponent->GetComponentLocation(), FColor::Red)

	//RootMotionCalculation
	float UpSpeed = Velocity | FVector::UpVector;
	float TransDistance = FVector::Dist(TransitionTarget, UpdatedComponent->GetComponentLocation());
	TransitionQueuedMontageSpeed = FMath::GetMappedRangeValueClamped(FVector2D(-500, 750), FVector2D(.9f, 1.2f), UpSpeed);
	TransitionRMS.Reset();
	TransitionRMS = MakeShared<FRootMotionSource_MoveToForce>();
	TransitionRMS->AccumulateMode = ERootMotionAccumulateMode::Override;
	TransitionRMS->Duration = FMath::Clamp(TransDistance / 500.f, .1f, .25f);
	SLOG(FString::Printf(TEXT("Duration: %f"), TransitionRMS->Duration))
	TransitionRMS->StartLocation = UpdatedComponent->GetComponentLocation();
	TransitionRMS->TargetLocation = TransitionTarget;

	//ApplyTransitionRootMotionSource
	Velocity = FVector::ZeroVector;
	SetMovementMode(MOVE_Flying);
	TransitionRMS_ID = ApplyRootMotionSource(TransitionRMS);

	//Animations
	if (bTallMantle)
	{
		
		TransitionQueueMontage = TallMantleMontage;
		CharacterOwner->PlayAnimMontage(TransitionTallMantleMontage, 1 / TransitionRMS->Duration);
		if (IsServer()) Proxy_bTallMantle = !Proxy_bTallMantle;
	
	}
	else
	{

		TransitionQueueMontage = ShortMantleMontage;
		CharacterOwner->PlayAnimMontage(TransitionShortMantleMontage, 1 / TransitionRMS->Duration);
		if (IsServer()) Proxy_bShortMantle = !Proxy_bShortMantle;
	
	}

	return true;

}

FVector UADMCharacterMovementComponent::GetMantelStartLocation(FHitResult FrontHit, FHitResult SurfaceHit, bool bTallMAntle)
{
	
	float CosWallStepnessAngle = FrontHit.Normal | FVector::UpVector;
	float DownDistance = bTallMAntle ? CapHH() * 2.f : MaxStepHeight - 1;
	FVector EdgeTrangent = FVector::CrossProduct(SurfaceHit.Normal, FrontHit.Normal).GetSafeNormal();
	FVector MantleStar = SurfaceHit.Location;
	MantleStar += FrontHit.Normal.GetSafeNormal2D() * (2.f + CapR());
	MantleStar += UpdatedComponent->GetForwardVector().GetSafeNormal2D().ProjectOnTo(EdgeTrangent) * CapR() * .3f;
	MantleStar += FVector::UpVector * CapHH();
	MantleStar += FVector::DownVector * DownDistance;
	MantleStar += FrontHit.Normal.GetSafeNormal2D() * CosWallStepnessAngle * DownDistance;
	return MantleStar;

}

void UADMCharacterMovementComponent::OnRep_TallMantle()
{

	CharacterOwner->PlayAnimMontage(ProxyTallMantleMontage);

}

void UADMCharacterMovementComponent::OnRep_ShortMantle()
{

	CharacterOwner->PlayAnimMontage(ProxyShortMantleMontage);

}

#pragma endregion

#pragma region HangFunctions

bool UADMCharacterMovementComponent::TryHang()
{
	if (!IsMovementMode(MOVE_Falling)) return false;

	SLOG("Wants to Hang")

	FHitResult WallHit;

	if (!GetWorld()->LineTraceSingleByProfile(WallHit, UpdatedComponent->GetComponentLocation(), UpdatedComponent->GetComponentLocation() + UpdatedComponent->GetForwardVector() * 300, "BlockAll", ADMCharacterOwner->GetIgnoreCharacterParams()))
	{

		return false;

	}

	TArray<FOverlapResult> OverlapResults;

	FVector ColLoc = UpdatedComponent->GetComponentLocation() + FVector::UpVector * CapHH() + UpdatedComponent->GetForwardVector() * CapR() * 3;
	auto ColBox = FCollisionShape::MakeBox(FVector(100, 100, 50));
	FQuat ColRot = FRotationMatrix::MakeFromXZ(WallHit.Normal, FVector::UpVector).ToQuat();

	if (!GetWorld()->OverlapMultiByChannel(OverlapResults, ColLoc, ColRot, ECC_WorldStatic, ColBox, ADMCharacterOwner->GetIgnoreCharacterParams()))
	{

		return false;

	}

	AActor* ClimbPoint = nullptr;

	float MaxHeight = -1e20;
	for (FOverlapResult Result : OverlapResults)
	{

		if (Result.GetActor()->ActorHasTag("Climb Point"))
		{

			float Height = Result.GetActor()->GetActorLocation().Z;
			if (Height > MaxHeight) {

				MaxHeight = Height;
				ClimbPoint = Result.GetActor();

			}

		}

	}

	if (!IsValid(ClimbPoint)) return false;

	FVector TargetLocation = ClimbPoint->GetActorLocation() + WallHit.Normal * CapR() * 1.01f + FVector::DownVector * CapHH();
	FQuat TargetRotation = FRotationMatrix::MakeFromXZ(-WallHit.Normal, FVector::UpVector).ToQuat();

	//TestIfCharacterCanRichGoal
	FTransform CurrentTransform = UpdatedComponent->GetComponentTransform();
	FHitResult Hit, ReturnHit;
	SafeMoveUpdatedComponent(TargetLocation - UpdatedComponent->GetComponentLocation(), UpdatedComponent->GetComponentQuat(), true, Hit);
	FVector ResultLocation = UpdatedComponent->GetComponentLocation();
	SafeMoveUpdatedComponent(CurrentTransform.GetLocation() - ResultLocation, TargetRotation, false, ReturnHit);
	if (!ResultLocation.Equals(TargetLocation)) return false;

	//PassedAllConditions
	bOrientRotationToMovement = false;

	//PerformTransitionToClimbPoint
	float UpSpeed = Velocity | FVector::UpVector;
	float TransDistance = FVector::Dist(TargetLocation, UpdatedComponent->GetComponentLocation());

	TransitionQueuedMontageSpeed = FMath::GetMappedRangeValueClamped(FVector2D(-500, 750), FVector2D(0.9f, 1.2f), UpSpeed);
	TransitionRMS.Reset();
	TransitionRMS = MakeShared<FRootMotionSource_MoveToForce>();
	TransitionRMS->AccumulateMode = ERootMotionAccumulateMode::Override;
	TransitionRMS->Duration = FMath::Clamp(TransDistance / 500.f, 0.1f, 0.25f);
	SLOG(FString::Printf(TEXT("Duration: %f"), TransitionRMS->Duration))
	TransitionRMS->StartLocation = UpdatedComponent->GetComponentLocation();
	TransitionRMS->TargetLocation = TargetLocation;

	//ApplyTransitionRootMotion
	Velocity = FVector::ZeroVector;
	SetMovementMode(MOVE_Falling);
	TransitionRMS_ID = ApplyRootMotionSource(TransitionRMS);

	//Anmations
	TransitionQueueMontage = nullptr;
	TransitionName = "Hang";
	CharacterOwner->PlayAnimMontage(TransitionHangMontage, 1 / TransitionRMS->Duration);

	return true;

}

#pragma endregion

#pragma region GlideFunctions

void UADMCharacterMovementComponent::Server_EnterGlide_Implementation()
{

	Safe_bWantsToGlide = true;

}

void UADMCharacterMovementComponent::EnterGlide(EMovementMode PrevMode, ECustomMovementMode PrevCustomMode)
{

	SLOG("Preping to glide");

	RecordGlideValues();

	RotationRate = GlideRotationRate;
	GravityScale = GlideGraveryScale;
	AirControl = GlideAirControl;
	MaxAcceleration = GlideMaxAccleration;
	bUseControllerDesiredRotation = true;

}

void UADMCharacterMovementComponent::ExitGlide()
{

	this->RotationRate = OriginalRotationRate;
	this->GravityScale = OriginalGraveryScale;
	this->AirControl = OriginalAirControl;
	this->MaxAcceleration = OriginalMaxAccleration;
	this->bUseControllerDesiredRotation = bOriginalDesiredRotation;

}

void UADMCharacterMovementComponent::RecordGlideValues()
{

	SLOG("Recording");

	OriginalRotationRate = this->RotationRate;
	OriginalGraveryScale = this->GravityScale;
	OriginalAirControl = this->AirControl;
	OriginalMaxAccleration = this->MaxAcceleration;
	bOriginalDesiredRotation = this->bUseControllerDesiredRotation;

}

bool UADMCharacterMovementComponent::CanGlide() const
{

	FHitResult Hit;
	FVector TraceStart = UpdatedComponent->GetComponentLocation();
	FVector TraceEnd = TraceStart + UpdatedComponent->GetUpVector() * GlideMinimumHeight * -1.f;
	auto Params = ADMCharacterOwner->GetIgnoreCharacterParams();

	GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params);

	if (Hit.bBlockingHit == false && this->IsFalling() == true)
	{

		return true;

	}
	else
	{

 		return false;

	}

}

void UADMCharacterMovementComponent::PhysGlide(float deltaTime, int32 Iterations)
{

	UE_LOG(LogTemp, Warning, TEXT("Scale: %f"), this->GravityScale);


	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	if (!CanGlide() && !IsGliding())
	{

		ExitGlide();

		SetMovementMode(MOVE_Walking);
		StartNewPhysics(deltaTime, Iterations);

	}

	bJustTeleported = false;
	bool bCheckedFall = false;
	bool bTriedLedgeMove = false;
	float remainingTime = deltaTime;

	FVector FallAcceleration = GetFallingLateralAcceleration(deltaTime);
	const FVector GravityRelativeFallAcceleration = RotateWorldToGravity(FallAcceleration);
	FallAcceleration = RotateGravityToWorld(FVector(GravityRelativeFallAcceleration.X, GravityRelativeFallAcceleration.Y, 0));
	const bool bHasLimitedAirControl = ShouldLimitAirControl(deltaTime, FallAcceleration);

	// Perform the move
	while ((remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations) && CharacterOwner && (CharacterOwner->Controller || bRunPhysicsWithNoController || (CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)))
	{

		Iterations++;
		bJustTeleported = false;
		const float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
		remainingTime -= timeTick;

		// Save current values
		UPrimitiveComponent* const OldBase = GetMovementBase();
		const FVector PreviousBaseLocation = (OldBase != NULL) ? OldBase->GetComponentLocation() : FVector::ZeroVector;
		FVector OldLocation = UpdatedComponent->GetComponentLocation();
		const FVector Adjusted = Velocity * deltaTime;
		FHitResult Hit(1.f);
		SafeMoveUpdatedComponent(Adjusted, UpdatedComponent->GetComponentQuat(), true, Hit);

		if (Hit.Time < 1.f)
		{

			const FVector GravDir = FVector(0.f, 0.f, -1.f);
			const FVector VelDir = Velocity.GetSafeNormal();
			const float UpDown = GravDir | VelDir;

			bool bSteppedUp = false;

			if ((FMath::Abs(Hit.ImpactNormal.Z) < 0.2f) && (UpDown < 0.5f) && (UpDown > -0.2f) && CanStepUp(Hit))
			{

				float stepZ = UpdatedComponent->GetComponentLocation().Z;
				bSteppedUp = StepUp(GravDir, Adjusted * (1.f - Hit.Time), Hit);

				if (bSteppedUp)
				{

					OldLocation.Z = UpdatedComponent->GetComponentLocation().Z + (OldLocation.Z - stepZ);

				}

			}

			if (!bSteppedUp)
			{

				//adjust and try again
				HandleImpact(Hit, deltaTime, Adjusted);
				SlideAlongSurface(Adjusted, (1.f - Hit.Time), Hit.Normal, Hit, true);

			}
		}

		if (!bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
		{

			Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime;

		}

		const FVector OldVelocity = Velocity;

		// Apply input
		const float MaxDecel = GetMaxBrakingDeceleration();
		if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
		{

			// Compute Velocity
			{

				// Acceleration = FallAcceleration for CalcVelocity(), but we restore it after using it.
				TGuardValue<FVector> RestoreAcceleration(Acceleration, FallAcceleration);

				Velocity.Z = UKismetMathLibrary::FInterpTo(Velocity.Z, GlideDescendngRate, timeTick, 3.f);
				this->Velocity.Z = GlideDescendngRate * -1.f;
				CalcVelocity(timeTick, FallingLateralFriction, false, MaxDecel);
				//UE_LOG(LogTemp, Warning, TEXT("Velocity: %f"), Velocity.Z);

			}

		}


		// Compute move parameters
		const FVector MoveVelocity = Velocity;
		const FVector Delta = timeTick * MoveVelocity; // dx = v * dt
		const bool bZeroDelta = Delta.IsNearlyZero();
		FStepDownResult StepDownResult;

		if (bZeroDelta)
		{

			remainingTime = 0.f;

		}
		else
		{

			if (IsWalking())
			{

				SetMovementMode(MOVE_Walking);
				return;

			}
			else if (IsSwimming()) //just entered water
			{

				StartSwimming(OldLocation, OldVelocity, timeTick, remainingTime, Iterations);
				return;

			}

		}

		FHitResult GroundHit;
		FVector TraceStart = UpdatedComponent->GetComponentLocation();
		FVector TraceEnd = TraceStart + UpdatedComponent->GetUpVector() * GlideMinimumHeight * -1.f;
		auto Params = ADMCharacterOwner->GetIgnoreCharacterParams();

		GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, Params);

		if (GroundHit.bBlockingHit == true && this->IsFalling() == false)
		{

			SetMovementMode(MOVE_Walking);

			StartNewPhysics(deltaTime, Iterations);

		}

	}

	if (IsMovingOnGround())
	{

		MaintainHorizontalGroundVelocity();

	}

}

#pragma endregion

#pragma region WallRunFunctions

bool UADMCharacterMovementComponent::TryWallRun()
{

	if (!IsFalling()) return false;

	if (Velocity.Z < -MaxVerticalWallRunSpeed) return false;

	if (Velocity.SizeSquared2D() < pow(MinWallRunSpeed, 2)) return false;

	FVector Start = UpdatedComponent->GetComponentLocation();
	FVector LeftEnd = Start - UpdatedComponent->GetRightVector() * CapR() * 2;
	FVector RightEnd = Start + UpdatedComponent->GetRightVector() * CapR() * 2;
	auto Params = ADMCharacterOwner->GetIgnoreCharacterParams();
	FHitResult FloorHit, WallHit;
	
	//CheckPlayerHeight
	if (GetWorld()->LineTraceSingleByProfile(FloorHit, Start, Start + FVector::DownVector * (CapHH() + MinWallRunHeight), "BlockAll", Params))
	{

		return false;

	}

	//LeftCast
	GetWorld()->LineTraceSingleByProfile(WallHit, Start, LeftEnd, "BlockAll", Params);

	if (WallHit.IsValidBlockingHit() && (Velocity | WallHit.Normal) < 0)
	{

		Safe_bWallRunIsRight = false;

	}
	else
	{

		//RightCast
		GetWorld()->LineTraceSingleByProfile(WallHit, Start, RightEnd, "BlockAll", Params);
		if (WallHit.IsValidBlockingHit() && (Velocity | WallHit.Normal) < 0)
		{

			Safe_bWallRunIsRight = true;

		}
		else
		{

			return false;

		}

	}

	FVector ProjectedVelocity = FVector::VectorPlaneProject(Velocity, WallHit.Normal);

	if (ProjectedVelocity.SizeSquared2D() < pow(MinWallRunSpeed, 2)) return false;

	//PassedAllConditons
	Velocity = ProjectedVelocity;
	Velocity.Z = FMath::Clamp(Velocity.Z, 0.f, MaxVerticalWallRunSpeed);

	SetMovementMode(MOVE_Custom, CMOVE_WallRun);

	//SLOG("Starting WallRun");

	return true;

}

void UADMCharacterMovementComponent::PhysWallRun(float deltaTime, int32 Iterations)
{

	if (deltaTime < MIN_TICK_TIME)
	{

		return;

	}

	if (!CharacterOwner || (!CharacterOwner && !bRunPhysicsWithNoController && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)))
	{

		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;

	}

	bJustTeleported = false;
	float remainingTime = deltaTime;

	//PerformTheMove
	while ((remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations) && CharacterOwner && (CharacterOwner->Controller || bRunPhysicsWithNoController || (CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)))
	{

		Iterations++;
		bJustTeleported = false;
		const float tickTime = GetSimulationTimeStep(remainingTime, Iterations);
		remainingTime -= tickTime;
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();

		FVector Start = UpdatedComponent->GetComponentLocation();
		FVector CastDelta = UpdatedComponent->GetRightVector() * CapR() * 2;
		FVector End = Safe_bWallRunIsRight ? Start + CastDelta : Start - CastDelta;
		auto Params = ADMCharacterOwner->GetIgnoreCharacterParams();
		float SinPullAwayAngle = FMath::Sin(FMath::DegreesToRadians(WallRunPullAwayAngle));
		FHitResult WallHit;
		
		GetWorld()->LineTraceSingleByProfile(WallHit, Start, End, "BlockAll", Params);

		bool bWantsToPullAway = WallHit.IsValidBlockingHit() && !Acceleration.IsNearlyZero() && (Acceleration.GetSafeNormal() | WallHit.Normal) > SinPullAwayAngle;

		if (!WallHit.IsValidBlockingHit() || bWantsToPullAway)
		{

			SetMovementMode(MOVE_Falling);
			StartNewPhysics(remainingTime, Iterations);

			return;

		}

		//ClampAccleration
		Acceleration = FVector::VectorPlaneProject(Acceleration, WallHit.Normal);
		Acceleration.Z = 0.f;

		//ApplyAccelerarion 
		CalcVelocity(tickTime, 0.f, false, GetMaxBrakingDeceleration());
		Velocity = FVector::VectorPlaneProject(Velocity, WallHit.Normal);
		float TangentAccel = Acceleration.GetSafeNormal() | Velocity.GetSafeNormal2D();
		bool bVelUp = Velocity.Z > 0.f;
		Velocity.Z += GetGravityZ() * WallRunGravityScaleCurve->GetFloatValue(bVelUp ? 0.f : TangentAccel) * tickTime;

		if (Velocity.SizeSquared2D() < pow(MinWallRunSpeed, 2) || Velocity.Z < -MaxVerticalWallRunSpeed)
		{

			SetMovementMode(MOVE_Falling);
			StartNewPhysics(remainingTime, Iterations);
			return;

		}

		//ComputeMoveParamiters
		const FVector Delta = tickTime * Velocity; // dx = v * dt
		const bool bZeroDelta = Delta.IsNearlyZero();

		if (bZeroDelta)
		{

			remainingTime = 0.f;

		}
		else
		{

			FHitResult Hit;

			SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, Hit);

			FVector WallAttractionDelta = -WallHit.Normal * WallAttractionForce * tickTime;

			SafeMoveUpdatedComponent(WallAttractionDelta, UpdatedComponent->GetComponentQuat(), true, Hit);

		}

		if (UpdatedComponent->GetComponentLocation() == OldLocation)
		{
			
			remainingTime = 0.f;
			break;

		}

		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / tickTime; // v = dx / dt

	}

	FVector Start = UpdatedComponent->GetComponentLocation();
	FVector CastDelta = UpdatedComponent->GetRightVector() * CapR() * 2;
	FVector End = Safe_bWallRunIsRight ? Start + CastDelta : Start - CastDelta;
	FVector FloorEnd = Start + FVector::DownVector * (CapHH() + MinWallRunHeight * .5f);
	auto Params = ADMCharacterOwner->GetIgnoreCharacterParams();
	FHitResult FloorHit, WallHit;

	GetWorld()->LineTraceSingleByProfile(WallHit, Start, End, "BlockAll", Params);
	GetWorld()->LineTraceSingleByProfile(FloorHit, Start, FloorEnd, "BlockAll", Params);

	if (FloorHit.IsValidBlockingHit() || !WallHit.IsValidBlockingHit() || Velocity.SizeSquared2D() < pow(MinWallRunSpeed, 2))
	{

		SetMovementMode(MOVE_Falling);

	}

}

#pragma endregion

#pragma region Helpers

bool UADMCharacterMovementComponent::IsCustomMovementMode(ECustomMovementMode InCustomMovementMode) const
{

	return MovementMode == MOVE_Custom && CustomMovementMode == InCustomMovementMode;

}

bool UADMCharacterMovementComponent::IsMovementMode(EMovementMode InMovementMode) const
{

	return InMovementMode == MovementMode;

}

bool UADMCharacterMovementComponent::IsServer()
{

	return CharacterOwner->HasAuthority();

}

float UADMCharacterMovementComponent::CapR() const
{

	return CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius();

}

float UADMCharacterMovementComponent::CapHH() const
{

	return CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

}

#pragma endregion

