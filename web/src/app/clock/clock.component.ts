import { Component, Input, OnInit } from '@angular/core';
import { Observable } from 'rxjs';

@Component({
    selector: 'clock',
    templateUrl: './clock.component.html',
    styleUrls: ['./clock.component.css']
})
export class ClockComponent implements OnInit {
    @Input('time') time$: Observable<Date>;
    constructor() { }

    ngOnInit(): void {
    }

}
