import { Component, OnInit, Inject, Input } from '@angular/core';
import { Observable } from 'rxjs';

@Component({
	selector: 'app-temperature-display',
	templateUrl: './temperature-display.component.html',
	styleUrls: ['./temperature-display.component.css']
})

export class TemperatureDisplayComponent implements OnInit {
	temp: number;
	@Input('temperatures') temperatures$: Observable<number>;
	constructor() {
	}
	ngOnInit() {
	}
}
