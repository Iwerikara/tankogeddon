// Fill out your copyright notice in the Description page of Project Settings.


#include "TankPawn.h"
#include <Components/StaticMeshComponent.h>
#include <Math/UnrealMathUtility.h>
#include <Kismet/KismetMathLibrary.h>
#include <Components/ArrowComponent.h>
#include "Tankogeddon.h"
#include "TankPlayerController.h"
#include "Cannon.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

// Sets default values
ATankPawn::ATankPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tank body"));
	RootComponent = BodyMesh;

	TurretMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tank turret"));
	TurretMesh->SetupAttachment(BodyMesh);

	CannonSetupPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("Cannon setup point"));
	CannonSetupPoint->AttachToComponent(TurretMesh, FAttachmentTransformRules::KeepRelativeTransform);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Spring arm"));
	SpringArm->SetupAttachment(BodyMesh);
	SpringArm->bDoCollisionTest = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritYaw = false;
	SpringArm->bInheritRoll = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("Health component"));
	HealthComponent->OnDie.AddDynamic(this, &ATankPawn::Die);
	HealthComponent->OnDamaged.AddDynamic(this, &ATankPawn::DamageTaken);

	HitCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("Hit collider"));
	HitCollider->SetupAttachment(BodyMesh);
}

void ATankPawn::MoveForward(float AxisValue)
{
	TargetForwardAxisValue = AxisValue;
}

void ATankPawn::RotateRight(float AxisValue)
{
	TargetRotateRightValue = AxisValue;
}

// Called when the game starts or when spawned
void ATankPawn::BeginPlay()
{
	Super::BeginPlay();

	TankController = Cast<ATankPlayerController>(GetController());
	SetupCannon(CannonClass);
}

void ATankPawn::SetupCannon(TSubclassOf<ACannon> SetupCannonClass)
{
	if (Cannon)
	{
		Cannon->Destroy();
		Cannon = nullptr;
	}

	CannonClass = SetupCannonClass;

	FActorSpawnParameters Params;
	Params.Instigator = this;
	Params.Owner = this;
	Cannon = GetWorld()->SpawnActor<ACannon>(CannonClass, Params);
	Cannon->AttachToComponent(CannonSetupPoint, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	AddCannon(SetupCannonClass);

}

TSubclassOf<ACannon> ATankPawn::GetCannonClass()
{
	return CannonClass;
}

ACannon* ATankPawn::GetCannon()
{
	return Cannon;
}

// Called every frame
void ATankPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CurrentForwardAxisValue = FMath::FInterpTo(CurrentForwardAxisValue, TargetForwardAxisValue, DeltaTime, MovementSmoothness);

	FVector CurrentLocation = GetActorLocation();
	FVector ForwardVector = GetActorForwardVector();
	FVector MovePosition = CurrentLocation + ForwardVector * CurrentForwardAxisValue * MoveSpeed * DeltaTime;

	SetActorLocation(MovePosition, true);

	// Tank rotation
	CurrentRightAxisValue = FMath::FInterpTo(CurrentRightAxisValue, TargetRotateRightValue, DeltaTime, RotationSmoothness);

	//UE_LOG(LogTankogeddon, VeryVerbose, TEXT("CurrentRightAxisValue = %f TargetRightAxisValue = %f"), CurrentRightAxisValue, TargetRightAxisValue);

	FRotator CurrentRotation = GetActorRotation();
	float YawRotation = CurrentRightAxisValue * RotationSpeed * DeltaTime;
	YawRotation += CurrentRotation.Yaw;

	FRotator NewRotation = FRotator(0.f, YawRotation, 0.f);
	SetActorRotation(NewRotation);

	// Turret rotation
	if (TankController)
	{
		FVector MousePos = TankController->GetMousePos();
		FRotator TargetRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), MousePos);
		FRotator CurrRotation = TurretMesh->GetComponentRotation();
		TargetRotation.Pitch = CurrRotation.Pitch;
		TargetRotation.Roll = CurrRotation.Roll;
		TurretMesh->SetWorldRotation(FMath::RInterpConstantTo(CurrRotation, TargetRotation, DeltaTime, TurretRotationSpeed));
	}
}

void ATankPawn::Fire()
{
	if (Cannon)
	{
		Cannon->Fire();
	}
}

void ATankPawn::AltFire()
{
	if (Cannon)
	{
		Cannon->AltFire();
	}
}

void ATankPawn::AddCannon(TSubclassOf<ACannon> CannonClassToAdd)
{
	if (CannonClassToAdd == FirstCannonClass)
	{
		bHasFirstCannon = true;
	}
	else if (CannonClassToAdd == SecondCannonClass)
	{
		bHasSecondCannon = true;
	}
	else if (CannonClassToAdd == ThirdCannonClass)
	{
		bHasThirdCannon = true;
	}
}

void ATankPawn::SelectFirstCannon()
{
	if (FirstCannonClass && bHasFirstCannon)
	{
		SetupCannon(FirstCannonClass);
	}
}

void ATankPawn::SelectSecondCannon()
{
	if (SecondCannonClass && bHasSecondCannon)
	{
		SetupCannon(SecondCannonClass);
	}
}

void ATankPawn::SelectThirdCannon()
{
	if (ThirdCannonClass && bHasThirdCannon)
	{
		SetupCannon(ThirdCannonClass);
	}
}

void ATankPawn::TakeDamage(FDamageData DamageData)
{
	HealthComponent->TakeDamage(DamageData);
}

void ATankPawn::Die()
{
	Destroy();
}

void ATankPawn::DamageTaken(float DamageValue)
{
	UE_LOG(LogTankogeddon, Warning, TEXT("Tank %s taked damage:%f Health:%f"), *GetName(), DamageValue, HealthComponent->GetHealth());
}