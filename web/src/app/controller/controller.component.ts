import { Component, OnInit, Inject, Input, SimpleChanges } from '@angular/core';
import * as Controller from '../controller.service';
import { Observable } from 'rxjs';

@Component({
	selector: 'app-controller',
	templateUrl: './controller.component.html',
	styleUrls: ['./controller.component.css']
})
export class ControllerComponent implements OnInit {
	@Input('status') status$: Observable<Controller.Status>;
	desiredTemperature: number;
	currentTemperature: number;
	power:boolean;
	modo: string;
	constructor(@Inject(Controller.ControllerService) public controller: Controller.ControllerService) {
	}
	ngOnInit() {
		this.status$.subscribe(x => {
			if (x.currentTemperature != null)
				this.currentTemperature = x.currentTemperature;
			this.power=x.power;
			if (x.targetTemperature != null) {
				this.modo = '2';
				if (this.desiredTemperature == null)
					this.desiredTemperature = x.targetTemperature;
			} else {
				this.desiredTemperature = null;
				if (x.power)
					this.modo = '1';
				else
					this.modo = '0';
			}
		});
	}
	Change($event) {
		console.log($event);
		let desired: Controller.Status = {
		};
		switch (this.modo) {
			case '0':
				desired.power = false;
				break;
			case '1':
				desired.power = true;
				break;
			case '2':
				if (this.desiredTemperature == null)
					this.desiredTemperature = this.currentTemperature;
				desired.targetTemperature = this.desiredTemperature;
				break;
			default:
				return;
		}
		console.warn(desired);
		this.controller.SendMode(desired);
	}
}
