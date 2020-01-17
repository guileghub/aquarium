import { Component, Inject, OnInit, OnDestroy } from '@angular/core';
import * as Controller from './controller.service';
import { Observable } from 'rxjs';
import { pluck } from 'rxjs/operators';

@Component({
	selector: 'app-root',
	templateUrl: './app.component.html',
	styleUrls: ['./app.component.css']
})
export class AppComponent implements OnInit, OnDestroy {
	baseUrl: string;
	title = 'web';
	temperatures$: Observable<number>;
	constructor(@Inject(Controller.ControllerService) public controller: Controller.ControllerService) {
		this.temperatures$ = this.controller.status$.pipe(pluck('currentTemperature'));
	}
	ngOnInit() {
		this.baseUrl = window.location.origin.replace("http:", "ws:") + ":81";
		//this.baseUrl = 'ws://192.168.4.1:81';
		this.Connect();
	}
	ngOnDestroy() {
	}
	Connect() {
		this.controller.connect(this.baseUrl);
	}
}