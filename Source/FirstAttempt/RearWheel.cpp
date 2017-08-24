// Fill out your copyright notice in the Description page of Project Settings.

#include "FirstAttempt.h"
#include "RearWheel.h"


URearWheel::URearWheel()
{
	ShapeRadius = 35.f;
	ShapeWidth = 10.0f;
	bAffectedByHandbrake = true;
	SteerAngle = 0.f;
}

