// Copyright, Bhargav Jt. Gogoi (Aarowan Vespera)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ADMCharacterMovementComponent.generated.h"

/**
 * 
 */

class AADMCharacter;

UENUM(BlueprintType)
enum ECustomMovementMode
{

	CMOVE_None			UMETA(Hidden),
	CMOVE_Hang			UMETA(DisplayName = "Hang"),
	CMOVE_Glide			UMETA(DisplayName = "Glide"),
	CMOVE_WallRun		UMETA(DisplayName = "Wall Run"),
	CMOVE_MAX			UMETA(Hidden),

};


UCLASS()
class AQUMDEMEMORIA_API UADMCharacterMovementComponent : public UCharacterMovementComponent
{

	GENERATED_BODY()
	
public:

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	UADMCharacterMovementComponent();

private:

#pragma region ReplicationLogic

	//SavedMoves
	class FSavedMove_ADM : public FSavedMove_Character
	{

		//Flags
		uint8 Saved_bADMPressedJump : 1;

		//Variables
		uint8 Saved_bHadAnimRootMotion : 1;
		uint8 Saved_bTransitionFinished : 1;

		uint8 Saved_bWantsToGlide : 1;

		uint8 Saved_bWallRunIsRight : 1;

		typedef FSavedMove_Character Super;

		virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
		virtual void Clear() override;
		virtual uint8 GetCompressedFlags() const override;
		virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData) override;
		virtual void PrepMoveFor(ACharacter* Character);

	};

	//NetworkPrediction
	class FNetworkPredictionData_Client_ADM : public FNetworkPredictionData_Client_Character
	{

	public:

		FNetworkPredictionData_Client_ADM(const UCharacterMovementComponent& ClientMovement);

		typedef FNetworkPredictionData_Client_Character Super;

		virtual FSavedMovePtr AllocateNewMove() override;

	};

public:

	virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	//ProxyReplications
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual float GetMaxSpeed() const override;
	virtual float GetMaxBrakingDeceleration() const override;

	virtual bool CanAttemptJump() const override;
	virtual bool DoJump(bool bReplayingMoves) override;

	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual void UpdateCharacterStateAfterMovement(float DeltaSeconds) override;

protected:

	//FlagUpdates
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual void OnClientCorrectionReceived(class FNetworkPredictionData_Client_Character& ClientData, float TimeStamp, FVector NewLocation, FVector NewVelocity, UPrimitiveComponent* NewBase, FName NewBaseBoneName, bool bHasBase, bool bBaseRelativePosition, uint8 ServerMovementMode, FVector ServerGravityDirection) override;
	virtual void InitializeComponent() override;

	virtual bool ServerCheckClientError(float ClientTimeStamp, float DeltaTime, const FVector& Accel, const FVector& ClientWorldLocation, const FVector& RelativeClientLocation, UPrimitiveComponent* ClientMovementBase, FName ClientBaseBoneName, uint8 ClientMovementMode) override;

	virtual void CallServerMovePacked(const FSavedMove_Character* NewMove, const FSavedMove_Character* PendingMove, const FSavedMove_Character* OldMove) override;

	FNetBitWriter ADMServerMoveBitWriter;

	virtual void PhysCustom(float deltaTime, int32 Iterations) override;

	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;


private:

	//Flags
	bool Safe_bHadAnimRootMotion;
	bool Safe_bTransitionFinished;

	bool Safe_bWantsToGlide;

	bool Safe_bWallRunIsRight;

	//SupportVariables
	TSharedPtr<FRootMotionSource_MoveToForce> TransitionRMS;
	FString TransitionName;
	UPROPERTY(Transient) UAnimMontage* TransitionQueueMontage;
	float TransitionQueuedMontageSpeed;
	int TransitionRMS_ID;
	UPROPERTY(Transient) AADMCharacter* ADMCharacterOwner;

	float AccumulatedClientLocationError = 0.f;

	int TickCount = 0;
	int CorrectionCount = 0;
	int TotalBitsSent = 0;

	//ReplicatedVariables
	UPROPERTY(ReplicatedUsing = OnRep_TallMantle) bool Proxy_bTallMantle;
	UPROPERTY(ReplicatedUsing = OnRep_ShortMantle) bool Proxy_bShortMantle;

#pragma endregion

#pragma region MantleVeriables

private:

	UPROPERTY(EditDefaultsOnly, category = "Mantle") float MantleMaxDistance = 200.f;
	UPROPERTY(EditDefaultsOnly, category = "Mantle") float MantleReachHeight = 50.f;
	UPROPERTY(EditDefaultsOnly, category = "Mantle") float MinMantoleDeapth = 30.f;
	UPROPERTY(EditDefaultsOnly, category = "Mantle") float MantoleMinWallSteepnessAngle = 75.f;
	UPROPERTY(EditDefaultsOnly, category = "Mantle") float MantoleMaxSurfaceAngle = 40.f;
	UPROPERTY(EditDefaultsOnly, category = "Mantle") float MantleMaxAlignmentAngle = 45.f;
	UPROPERTY(EditDefaultsOnly, category = "Mantle") UAnimMontage* TallMantleMontage;
	UPROPERTY(EditDefaultsOnly, category = "Mantle") UAnimMontage* TransitionTallMantleMontage;
	UPROPERTY(EditDefaultsOnly, category = "Mantle") UAnimMontage* ProxyTallMantleMontage;
	UPROPERTY(EditDefaultsOnly, category = "Mantle") UAnimMontage* ShortMantleMontage;
	UPROPERTY(EditDefaultsOnly, category = "Mantle") UAnimMontage* TransitionShortMantleMontage;
	UPROPERTY(EditDefaultsOnly, category = "Mantle") UAnimMontage* ProxyShortMantleMontage;

#pragma endregion

#pragma region MantleFunctions

private:

	bool TryMantle();
	FVector GetMantelStartLocation(FHitResult FrontHit, FHitResult SurfaceHit, bool bTallMantle);

	//Replication
	UFUNCTION() void OnRep_TallMantle();
	UFUNCTION() void OnRep_ShortMantle();


#pragma endregion

#pragma region HangVariables

	UPROPERTY(EditDefaultsOnly, category = "Hang") UAnimMontage* TransitionHangMontage;
	UPROPERTY(EditDefaultsOnly, category = "Hang") UAnimMontage* WallJumpMontage;
	UPROPERTY(EditDefaultsOnly, category = "Hang") float WallJumpForce = 400.f;

#pragma endregion

#pragma region HangFunctions

	bool TryHang();
	UFUNCTION(BlueprintPure) bool IsHanging() const { return IsCustomMovementMode(CMOVE_Hang); }

#pragma endregion

#pragma region GlideVariables

	UPROPERTY() FRotator OriginalRotationRate;
	UPROPERTY() float OriginalGraveryScale;
	UPROPERTY() float OriginalAirControl;
	UPROPERTY() float OriginalMaxAccleration;
	UPROPERTY() bool bOriginalDesiredRotation;

	UPROPERTY(EditDefaultsOnly, category = "Glide") FRotator GlideRotationRate = FRotator(0.f, 250.f, 0.f);
	UPROPERTY(EditDefaultsOnly, category = "Glide") float GlideGraveryScale = 0.f;
	UPROPERTY(EditDefaultsOnly, category = "Glide") float GlideAirControl = 0.9f;
	UPROPERTY(EditDefaultsOnly, category = "Glide") float GlideMaxAccleration = 1024.f;
	UPROPERTY(EditDefaultsOnly, category = "Glide") float GlideMaxWalkSpeed = 600.f;

	UPROPERTY(EditDefaultsOnly, category = "Glide") float GlideMinimumHeight = 300.f;
	UPROPERTY(EditDefaultsOnly, category = "Glide") float GlideDescendngRate = 300.f;

#pragma endregion

#pragma region GlideFunctions

	UFUNCTION(Server, Reliable) void Server_EnterGlide();

	void EnterGlide(EMovementMode PrevMode, ECustomMovementMode PrevCustomMode);
	void ExitGlide();
	void RecordGlideValues();
	bool CanGlide() const;
	void PhysGlide(float deltaTime, int32 Iterations);
	UFUNCTION(BlueprintPure) bool IsGliding() const { return IsCustomMovementMode(CMOVE_Glide); }

#pragma endregion

#pragma region WallRunVariables
	 
	UPROPERTY(EditDefaultsOnly, category = "WallRun") float MinWallRunSpeed = 200.f;
	UPROPERTY(EditDefaultsOnly, category = "WallRun") float MaxWallRunSpeed = 800.f;
	UPROPERTY(EditDefaultsOnly, category = "WallRun") float MaxVerticalWallRunSpeed = 200.f;
	UPROPERTY(EditDefaultsOnly, category = "WallRun") float WallRunPullAwayAngle = 75.f;
	UPROPERTY(EditDefaultsOnly, category = "WallRun") float WallAttractionForce = 200.f;
	UPROPERTY(EditDefaultsOnly, category = "WallRun") float MinWallRunHeight = 50.f;
	UPROPERTY(EditDefaultsOnly, category = "WallRun") float WallJumpOffForce= 300.f;
	UPROPERTY(EditDefaultsOnly, category = "WallRun") UCurveFloat* WallRunGravityScaleCurve;

#pragma endregion

#pragma region WallRunFunctions

	bool TryWallRun();
	void PhysWallRun(float deltaTime, int32 Iterations);
	UFUNCTION(BlueprintPure) bool IsWallRunning() const { return IsCustomMovementMode(CMOVE_WallRun); }
	UFUNCTION(BlueprintPure) bool IsWallRunningRight() const { return Safe_bWallRunIsRight;  }

#pragma endregion

#pragma region Helpers

	UFUNCTION(BlueprintPure) bool IsCustomMovementMode(ECustomMovementMode InCustomMovementMode) const;
	UFUNCTION(BlueprintPure) bool IsMovementMode(EMovementMode InMovementMode) const;

	//Helpers
	bool IsServer();
	float CapR() const;
	float CapHH() const;

#pragma endregion

};
