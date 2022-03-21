import { Component, Input, Inject, OnInit } from '@angular/core';
import * as Controller from '../controller.service';
import { Observable } from 'rxjs';

@Component({
    selector: 'app-log',
    templateUrl: './log.component.html',
    styleUrls: ['./log.component.css']
})
export class LogComponent implements OnInit {
    @Input('logs') logs$: Observable<string>;
    logs: string[] = [];

    constructor() {
    }

    ngOnInit() {
        this.logs$.subscribe(x => {
            this.logs.push(x);
            while (this.logs.length > 10)
                this.logs.shift();
        });
    }
}
