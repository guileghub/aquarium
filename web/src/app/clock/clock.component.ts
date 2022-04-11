import { Component, Input, OnInit } from '@angular/core';
import { Observable } from 'rxjs';
import { Time } from '../time';
@Component({
    selector: 'clock',
    templateUrl: './clock.component.html',
    styleUrls: ['./clock.component.css']
})
export class ClockComponent implements OnInit {
    @Input('time') time$: Observable<Time>;
    uptime = "";
    time: Date = new Date();
    constructor() { }

    ngOnInit(): void {
        this.time$.subscribe(x => {
            if (x.time)
                this.time = x.time;
            this.uptime = "";
            if (x.uptime.days)
                this.uptime += x.uptime.days + " days ";
            if (x.uptime.hours)
                this.uptime += x.uptime.hours + " hours ";
            if (x.uptime.minutes)
                this.uptime += x.uptime.minutes + " minutes ";
            if (x.uptime.seconds)
                this.uptime += x.uptime.seconds + " seconds ";
        })
    }

}
