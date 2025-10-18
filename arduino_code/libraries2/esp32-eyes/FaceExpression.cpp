/***************************************************
Copyright (c) 2020 Luis Llamas
(www.luisllamas.es)

This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License along with this program.  If not, see <http://www.gnu.org/licenses
****************************************************/

#include "FaceExpression.h"
#include "Face.h"

FaceExpression::FaceExpression(Face &face) : _face(face)
{
}

static void SetEyeConfigSize(EyeConfig *conf, uint8_t size, int8_t width)
{  
	conf->OffsetX = (conf->OffsetX * size + 5) / 4;
	conf->OffsetY = (conf->OffsetY * size + 5) / 4;

	conf->Height = (conf->Height * size + 5) / 4;
	conf->Width = (conf->Width * size + 5) / 4 + width; 

	conf->Slope_Top = conf->Slope_Top * size;
	conf->Slope_Bottom = conf->Slope_Bottom * size;

	conf->Radius_Top = (conf->Radius_Top * size + 5) / 4;
	conf->Radius_Bottom = (conf->Radius_Bottom * size + 5) / 4;

	conf->Inverse_Radius_Top = (conf->Inverse_Radius_Top * size + 5) / 4;
	conf->Inverse_Radius_Bottom = (conf->Inverse_Radius_Bottom * size + 5) / 4;

	conf->Inverse_Offset_Top = (conf->Inverse_Offset_Top * size + 5) / 4;
	conf->Inverse_Offset_Bottom = (conf->Inverse_Offset_Bottom * size + 5) / 4;
}

static void SetEyeConfigOffset(EyeConfig *conf, uint8_t x)
{
	int Width = conf->Width;
	conf->OffsetX = (conf->OffsetX * Width + x) / 4;
}
 
void FaceExpression::SetFaceSize(uint8_t size, int8_t width)
{
	SetEyeConfigSize(&Preset_Normal, size, width);
	SetEyeConfigSize(&Preset_Happy, size, width);
	SetEyeConfigSize(&Preset_Glee, size, width);
	// SetEyeConfigSize(&Preset_Sad, size, width);
	// SetEyeConfigSize(&Preset_Worried, size, width);
	// SetEyeConfigSize(&Preset_Worried_Alt, size, width);
	SetEyeConfigSize(&Preset_Focused, size, width);
	SetEyeConfigSize(&Preset_Annoyed, size, width);
	SetEyeConfigSize(&Preset_Annoyed_Alt, size, width);
	// SetEyeConfigSize(&Preset_Surprised, size, width);
	// SetEyeConfigSize(&Preset_Skeptic, size, width);
	// SetEyeConfigSize(&Preset_Skeptic_Alt, size, width);
	SetEyeConfigSize(&Preset_Frustrated, size, width);
	// SetEyeConfigSize(&Preset_Unimpressed, size, width);
	// SetEyeConfigSize(&Preset_Unimpressed_Alt, size, width);
	// SetEyeConfigSize(&Preset_Sleepy, size, width);
	// SetEyeConfigSize(&Preset_Sleepy_Alt, size, width);
	// SetEyeConfigSize(&Preset_Suspicious, size, width);
	// SetEyeConfigSize(&Preset_Suspicious_Alt, size, width);
	SetEyeConfigSize(&Preset_Squint, size, width);
	SetEyeConfigSize(&Preset_Squint_Alt, size, width);
	// SetEyeConfigSize(&Preset_Angry, size, width);
	// SetEyeConfigSize(&Preset_Furious, size, width);
	// SetEyeConfigSize(&Preset_Scared, size, width);
	// SetEyeConfigSize(&Preset_Awe, size, width);
}

void FaceExpression::ClearVariations()
{
	_face.RightEye.Variation1.Clear();
	_face.RightEye.Variation2.Clear();
	_face.LeftEye.Variation1.Clear();
	_face.LeftEye.Variation2.Clear();
	_face.RightEye.Variation1.Animation.Restart();
	_face.LeftEye.Variation1.Animation.Restart();
}

void FaceExpression::GoTo_Normal()
{
	ClearVariations();

	_face.RightEye.Variation1.Values.Height = 3;
	_face.RightEye.Variation2.Values.Width = 1;
	_face.LeftEye.Variation1.Values.Height = 2;
	_face.LeftEye.Variation2.Values.Width = 2;
	_face.RightEye.Variation1.Animation.SetTriangle(1000, 0);
	_face.LeftEye.Variation1.Animation.SetTriangle(1000, 0);

	_face.RightEye.TransitionTo(Preset_Normal);
	_face.LeftEye.TransitionTo(Preset_Normal);
}

void FaceExpression::GoTo_Angry()
{
	ClearVariations();
	_face.RightEye.Variation1.Values.OffsetY = 2;
	_face.LeftEye.Variation1.Values.OffsetY = 2;
	_face.RightEye.Variation1.Animation.SetTriangle(300, 0);
	_face.LeftEye.Variation1.Animation.SetTriangle(300, 0);

	_face.RightEye.TransitionTo(Preset_Angry);
	_face.LeftEye.TransitionTo(Preset_Angry);
}

void FaceExpression::GoTo_Glee()
{
	ClearVariations();
	_face.RightEye.Variation1.Values.OffsetY = 5;
	_face.LeftEye.Variation1.Values.OffsetY = 5;
	_face.RightEye.Variation1.Animation.SetTriangle(300, 0);
	_face.LeftEye.Variation1.Animation.SetTriangle(300, 0);

	_face.RightEye.TransitionTo(Preset_Glee);
	_face.LeftEye.TransitionTo(Preset_Glee);
}

void FaceExpression::GoTo_Happy()
{
	ClearVariations();
	_face.RightEye.TransitionTo(Preset_Happy);
	_face.LeftEye.TransitionTo(Preset_Happy);
}

void FaceExpression::GoTo_Sad()
{
	ClearVariations();
	_face.RightEye.TransitionTo(Preset_Sad);
	_face.LeftEye.TransitionTo(Preset_Sad);
}

void FaceExpression::GoTo_Worried()
{
	ClearVariations();
	_face.RightEye.TransitionTo(Preset_Worried);
	_face.LeftEye.TransitionTo(Preset_Worried_Alt);
}

void FaceExpression::GoTo_Focused()
{
	ClearVariations();
	_face.RightEye.TransitionTo(Preset_Focused);
	_face.LeftEye.TransitionTo(Preset_Focused);
}

void FaceExpression::GoTo_Annoyed()
{
	ClearVariations();
	_face.RightEye.TransitionTo(Preset_Annoyed);
	_face.LeftEye.TransitionTo(Preset_Annoyed_Alt);
}

void FaceExpression::GoTo_Surprised()
{
	ClearVariations();
	_face.RightEye.TransitionTo(Preset_Surprised);
	_face.LeftEye.TransitionTo(Preset_Surprised);
}

void FaceExpression::GoTo_Skeptic()
{
	ClearVariations();
	_face.RightEye.TransitionTo(Preset_Skeptic);
	_face.LeftEye.TransitionTo(Preset_Skeptic_Alt);
}

void FaceExpression::GoTo_Frustrated()
{
	ClearVariations();
	_face.RightEye.TransitionTo(Preset_Frustrated);
	_face.LeftEye.TransitionTo(Preset_Frustrated);
}

void FaceExpression::GoTo_Unimpressed()
{
	ClearVariations();
	_face.RightEye.TransitionTo(Preset_Unimpressed);
	_face.LeftEye.TransitionTo(Preset_Unimpressed_Alt);
}

void FaceExpression::GoTo_Sleepy()
{
	ClearVariations();
	_face.RightEye.TransitionTo(Preset_Sleepy);
	_face.LeftEye.TransitionTo(Preset_Sleepy_Alt);
}

void FaceExpression::GoTo_Suspicious()
{
	ClearVariations();
	_face.RightEye.TransitionTo(Preset_Suspicious);
	_face.LeftEye.TransitionTo(Preset_Suspicious_Alt);
}

void FaceExpression::GoTo_Squint()
{
	ClearVariations();

	_face.LeftEye.Variation1.Values.OffsetX = 6;
	_face.LeftEye.Variation2.Values.OffsetY = 6;

	_face.RightEye.TransitionTo(Preset_Squint);
	_face.LeftEye.TransitionTo(Preset_Squint_Alt);
}

void FaceExpression::GoTo_Furious()
{
	ClearVariations();

	_face.RightEye.TransitionTo(Preset_Furious);
	_face.LeftEye.TransitionTo(Preset_Furious);
}

void FaceExpression::GoTo_Scared()
{
	ClearVariations();

	_face.RightEye.TransitionTo(Preset_Scared);
	_face.LeftEye.TransitionTo(Preset_Scared);
}

void FaceExpression::GoTo_Awe()
{
	ClearVariations();

	_face.RightEye.TransitionTo(Preset_Awe);
	_face.LeftEye.TransitionTo(Preset_Awe);
}
