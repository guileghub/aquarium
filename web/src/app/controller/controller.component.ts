import { Component, OnInit, Inject } from '@angular/core';
import * as Controller from '../controller.service';

@Component({
	selector: 'app-controller',
	templateUrl: './controller.component.html',
	styleUrls: ['./controller.component.css']
})
export class ControllerComponent implements OnInit {
	temperature: number;
	modo: string;
	constructor(@Inject(Controller.ControllerService) public controller: Controller.ControllerService) {
	}
	ngOnInit() {
	}
}
