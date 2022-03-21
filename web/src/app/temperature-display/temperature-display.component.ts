import { Component, OnInit, Input } from '@angular/core';
import { Observable } from 'rxjs';

@Component({
	selector: 'app-temperature-display',
	templateUrl: './temperature-display.component.html',
	styleUrls: ['./temperature-display.component.css']
})

export class TemperatureDisplayComponent implements OnInit {
	@Input('temperatures') temperatures$: Observable<number>;
	constructor() {
	}
	ngOnInit() {
		//this.temperatures$.subscribe(x=>{console.warn((x))});
	}
}
