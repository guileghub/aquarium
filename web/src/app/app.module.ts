import { NgModule } from '@angular/core';

import { CommonModule } from '@angular/common';
import { BrowserModule } from '@angular/platform-browser';
import { MatInputModule } from '@angular/material/input';
import { FormsModule/*, ReactiveFormsModule*/ } from '@angular/forms';
import { MatButtonModule } from '@angular/material/button';
/*import { MatSliderModule } from '@angular/material/slider';*/

/*import {A11yModule} from '@angular/cdk/a11y';
import {DragDropModule} from '@angular/cdk/drag-drop';
import {PortalModule} from '@angular/cdk/portal';
import {ScrollingModule} from '@angular/cdk/scrolling';
import {CdkStepperModule} from '@angular/cdk/stepper';*/
//import { BrowserAnimationsModule } from '@angular/platform-browser/animations';
const angularmods = [CommonModule, BrowserModule, MatInputModule, FormsModule, /*ReactiveFormsModule,*/ MatButtonModule
/*,
A11yModule, 
DragDropModule,
PortalModule,
ScrollingModule, 
CdkStepperModule,
MatSliderModule*/
];

import { MaterialModule } from './material-module';

import { AppComponent } from './app.component';
import { TemperatureDisplayComponent } from './temperature-display/temperature-display.component';
import { BrowserAnimationsModule } from '@angular/platform-browser/animations';
import { LogComponent } from './log/log.component';
import { PowerControlComponent } from './power-control/power-control.component';

@NgModule({
	declarations: [
		AppComponent,
		TemperatureDisplayComponent,
        LogComponent,
        PowerControlComponent
	],
	imports: [
		...angularmods,
		BrowserAnimationsModule,
		MaterialModule
	],
	providers: [],
	bootstrap: [AppComponent]
})
export class AppModule { }
