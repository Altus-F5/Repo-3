#include "RingCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "RingBodyComponent.h"
#include "RingLogic.h"
#include "RingSynthComponent.h"
#include "RingTKObject.h"
#include "RingTuning.h"

namespace
{
	const float CapsuleRadius = 34.f;
	const float CapsuleHalfHeight = 88.f;
	/** Height of the chest above the capsule centre: where held objects are measured from. */
	const float ChestOffset = 45.f;
	/** Held objects stay between these heights above the feet. */
	const float HoldFloorClearance = 15.f;
	const float HoldCeiling = 330.f;
	/** A held object farther than range times this is let go. */
	const float LoseHoldFactor = 1.25f;
	const float NoticeSeconds = 2.5f;
}

ARingCharacter::ARingCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(CapsuleRadius, CapsuleHalfHeight);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.f, 420.f, 0.f);
	Movement->MaxWalkSpeed = 230.f;
	Movement->BrakingDecelerationWalking = 1400.f;
	Movement->MaxAcceleration = 1200.f;

	Boom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Boom"));
	Boom->SetupAttachment(GetCapsuleComponent());
	Boom->SetRelativeLocation(FVector(0.0, 0.0, 58.0));
	Boom->TargetArmLength = 330.f;
	Boom->bUsePawnControlRotation = true;
	Boom->bDoCollisionTest = true;
	Boom->ProbeSize = 14.f;
	Boom->ProbeChannel = ECC_Camera;
	Boom->bEnableCameraLag = true;
	Boom->CameraLagSpeed = 12.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Boom, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	Body = CreateDefaultSubobject<URingBodyComponent>(TEXT("Body"));
	Body->SetupAttachment(GetCapsuleComponent());
	Body->SetRelativeLocation(FVector(0.0, 0.0, -CapsuleHalfHeight));

	SuitLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("SuitLight"));
	SuitLight->SetupAttachment(GetCapsuleComponent());
	SuitLight->SetRelativeLocation(FVector(16.0, 0.0, 38.0));
	SuitLight->SetRelativeRotation(FRotator(-6.f, 0.f, 0.f));
	SuitLight->SetIntensityUnits(ELightUnits::Candelas);
	SuitLight->SetIntensity(0.f);
	SuitLight->SetAttenuationRadius(1800.f);
	SuitLight->SetInnerConeAngle(18.f);
	SuitLight->SetOuterConeAngle(38.f);
	SuitLight->SetLightColor(FLinearColor(0.92f, 0.96f, 1.0f));

	Synth = CreateDefaultSubobject<URingSynthComponent>(TEXT("Synth"));
	Synth->SetupAttachment(GetCapsuleComponent());
}

void ARingCharacter::BeginPlay()
{
	Super::BeginPlay();
	const URingTuning& Tuning = URingTuning::Get();
	HoldDistance = Tuning.HoldDistanceDefault;
	Body->Build(ERingBodyLook::Elysia);
	Synth->Start();

	// Auto exposure stays on so darkness adapts like an eye would, but it is
	// clamped so an unpowered room cannot be lifted to grey.
	FPostProcessSettings& Post = Camera->PostProcessSettings;
	Post.bOverride_AutoExposureMinBrightness = true;
	Post.AutoExposureMinBrightness = Tuning.ExposureMinEV;
	Post.bOverride_AutoExposureMaxBrightness = true;
	Post.AutoExposureMaxBrightness = Tuning.ExposureMaxEV;
	Post.bOverride_AutoExposureBias = true;
	Post.AutoExposureBias = ExposureBias;
	Post.bOverride_MotionBlurAmount = true;
	Post.MotionBlurAmount = 0.f;
	Camera->PostProcessBlendWeight = 1.f;
}

void ARingCharacter::MoveInput(const FVector2D& Axis)
{
	if (!Controller || Axis.IsNearlyZero())
	{
		return;
	}
	const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Axis.X);
}

void ARingCharacter::ToggleHold()
{
	if (HeldObject)
	{
		ForceRelease();
		return;
	}
	if (!IsFocusing())
	{
		ShowNotice(TEXT("Focus first, then take hold."));
		return;
	}
	if (!TargetObject)
	{
		ShowNotice(TEXT("Nothing in reach to take hold of."));
		return;
	}
	if (!TargetBlockedReason.IsEmpty())
	{
		ShowNotice(TargetBlockedReason); // out of reach, braced, or being stood on
		return;
	}
	FString Reason;
	if (!TargetObject->CanAcquire(Reason))
	{
		ShowNotice(Reason);
		return;
	}
	HeldObject = TargetObject;
	TargetObject = nullptr;
	TargetBlockedReason.Reset();
	// Start from where the object already is, so it does not lurch when taken.
	const URingTuning& Tuning = URingTuning::Get();
	const float Current = static_cast<float>(FVector::Dist(GetActorLocation() + FVector(0.0, 0.0, ChestOffset), HeldObject->GetActorLocation()));
	HoldDistance = FMath::Clamp(Current, Tuning.HoldDistanceMin, Tuning.HoldDistanceMax);
	LastImpactCount = HeldObject->GetImpactCount();
	HeldObject->Acquire();
	Synth->TriggerClick();
}

void ARingCharacter::ForceRelease()
{
	if (HeldObject)
	{
		HeldObject->Release();
		if (HeldObject->GetImpactCount() != LastImpactCount)
		{
			Synth->TriggerThud();
		}
		HeldObject = nullptr;
		Synth->TriggerClick();
	}
}

void ARingCharacter::AdjustHoldDistance(float Delta)
{
	if (!HeldObject)
	{
		return;
	}
	const URingTuning& Tuning = URingTuning::Get();
	HoldDistance = RingLogic::AdjustHoldDistance(HoldDistance, Delta, Tuning.HoldDistanceStep, Tuning.HoldDistanceMin, Tuning.HoldDistanceMax);
}

void ARingCharacter::SetReducedMotion(bool bReduced)
{
	bReducedMotion = bReduced;
	Boom->bEnableCameraLag = !bReduced;
}

void ARingCharacter::SetExposureBias(float Stops)
{
	ExposureBias = Stops;
	Camera->PostProcessSettings.bOverride_AutoExposureBias = true;
	Camera->PostProcessSettings.AutoExposureBias = Stops;
}

void ARingCharacter::SetDroneLit(bool bLit)
{
	Body->SetDroneLit(bLit);
}

void ARingCharacter::ShowNotice(const FString& Text)
{
	Notice = Text;
	NoticeTime = NoticeSeconds;
}

void ARingCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const URingTuning& Tuning = URingTuning::Get();

	if (NoticeTime > 0.f)
	{
		NoticeTime -= DeltaSeconds;
		if (NoticeTime <= 0.f)
		{
			Notice.Reset();
		}
	}

	// Movement mode. While focusing she faces where the camera looks and does not hurry.
	const bool bFocusing = IsFocusing();
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	bUseControllerRotationYaw = bFocusing;
	Movement->bOrientRotationToMovement = !bFocusing;
	Movement->MaxWalkSpeed = HeldObject ? Tuning.CarrySpeed : ((bBrisk && !bFocusing) ? Tuning.BriskSpeed : Tuning.WalkSpeed);

	UpdateTarget();
	UpdateHeld(DeltaSeconds);
	UpdateCamera(DeltaSeconds);

	// Body animation and footsteps.
	const float GroundSpeed = static_cast<float>(GetVelocity().Size2D());
	Body->SetLocomotion(GroundSpeed);
	Body->SetPowerPose(HeldObject ? 1.f : (bFocusing ? 0.45f : 0.f));
	if (Body->GetStepCount() != LastStepCount)
	{
		LastStepCount = Body->GetStepCount();
		Synth->TriggerFootstep();
	}

	// Suit light fades rather than snaps.
	SuitLightLevel = FMath::FInterpTo(SuitLightLevel, bSuitLightOn ? 1.f : 0.f, DeltaSeconds, 3.f);
	SuitLight->SetIntensity(Tuning.SuitLightCandela * SuitLightLevel);
	SuitLight->SetVisibility(SuitLightLevel > 0.01f);
	SuitLight->SetRelativeRotation(FRotator(FMath::Clamp(static_cast<float>(FRotator::NormalizeAxis(GetControlRotation().Pitch)), -35.f, 25.f) - 6.f, 0.f, 0.f));
}

void ARingCharacter::UpdateTarget()
{
	TargetObject = nullptr;
	TargetBlockedReason.Reset();
	if (HeldObject || !bFocusInput)
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const URingTuning& Tuning = URingTuning::Get();
	const FVector Start = Camera->GetComponentLocation();
	const FVector End = Start + Camera->GetForwardVector() * (Tuning.TelekinesisRange + Boom->TargetArmLength);
	FCollisionQueryParams Params(FName(TEXT("RingTKTarget")), false, this);

	// A straight ray first; if it misses a marked object, a thicker sweep makes aiming forgiving.
	ARingTKObject* Found = nullptr;
	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		Found = Cast<ARingTKObject>(Hit.GetActor());
	}
	if (!Found && World->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(22.f), Params))
	{
		Found = Cast<ARingTKObject>(Hit.GetActor());
	}
	if (!Found)
	{
		return;
	}
	if (FVector::Dist(GetActorLocation(), Hit.ImpactPoint) > Tuning.TelekinesisRange)
	{
		TargetBlockedReason = TEXT("Out of reach. Move closer.");
		TargetObject = Found;
		return;
	}
	TargetObject = Found;
	FString Reason;
	if (!Found->CanAcquire(Reason))
	{
		TargetBlockedReason = Reason;
	}
}

void ARingCharacter::UpdateHeld(float DeltaSeconds)
{
	if (!HeldObject)
	{
		Synth->SetHum(0.f);
		Synth->SetGrind(0.f);
		return;
	}
	const URingTuning& Tuning = URingTuning::Get();
	const FVector Chest = GetActorLocation() + FVector(0.0, 0.0, ChestOffset);

	// Out of range, or the route to it is gone: let go rather than drag it through anything.
	if (FVector::Dist(Chest, HeldObject->GetActorLocation()) > Tuning.TelekinesisRange * LoseHoldFactor)
	{
		ForceRelease();
		ShowNotice(TEXT("Too far. Lost hold."));
		return;
	}

	FVector Target = Chest + Camera->GetForwardVector() * HoldDistance;
	const double Feet = GetActorLocation().Z - CapsuleHalfHeight;
	Target.Z = FMath::Clamp(Target.Z, Feet + HoldFloorClearance + HeldObject->GetSizeCm().Z * 0.5, Feet + HoldCeiling);
	HeldObject->SetHoldTarget(Target);

	if (HeldObject->GetImpactCount() != LastImpactCount)
	{
		LastImpactCount = HeldObject->GetImpactCount();
		Synth->TriggerThud();
	}
	Synth->SetHum(0.35f + 0.65f * HeldObject->GetMotionAmount());
	Synth->SetGrind(HeldObject->IsTrack() ? HeldObject->GetMotionAmount() : 0.f);
}

void ARingCharacter::UpdateCamera(float DeltaSeconds)
{
	const URingTuning& Tuning = URingTuning::Get();
	const bool bFocusing = IsFocusing();
	// Reduced motion: slower, gentler framing changes and no change of field of view.
	const float BlendSpeed = bReducedMotion ? 3.f : 7.f;
	FocusAlpha = FMath::FInterpTo(FocusAlpha, bFocusing ? 1.f : 0.f, DeltaSeconds, BlendSpeed);
	ShoulderSmooth = FMath::FInterpTo(ShoulderSmooth, ShoulderSide, DeltaSeconds, BlendSpeed);

	Boom->TargetArmLength = FMath::Lerp(Tuning.CameraArmFree, Tuning.CameraArmFocus, FocusAlpha);
	const float Side = FMath::Lerp(Tuning.CameraShoulderOffset * 0.6f, Tuning.CameraShoulderOffset, FocusAlpha) * ShoulderSmooth;
	Boom->SocketOffset = FVector(0.0, Side, FMath::Lerp(8.f, 16.f, FocusAlpha));
	Camera->SetFieldOfView(bReducedMotion ? Tuning.CameraFovFree : FMath::Lerp(Tuning.CameraFovFree, Tuning.CameraFovFocus, FocusAlpha));
}
