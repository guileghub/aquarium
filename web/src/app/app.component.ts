import { Component, Inject, OnInit } from '@angular/core';
import * as Controller from './controller.service';

@Component({
	selector: 'app-root',
	templateUrl: './app.component.html',
	styleUrls: ['./app.component.css']
})
export class AppComponent implements OnInit {
	baseUrl: string;
	title = 'web';
	constructor(@Inject(Controller.ControllerService) public controller: Controller.ControllerService) {
	}
	ngOnInit() {
		this.baseUrl = window.location.origin.replace("http:", "ws:").replace(':80', ':81');
		this.baseUrl = 'ws://192.168.4.1:81';
		this.Connect();
	}
	Connect() {
		this.controller.connect(this.baseUrl);
	}
}