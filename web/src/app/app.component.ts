import { Component, Inject, OnInit, OnDestroy } from '@angular/core';
import { ControllerService } from './controller.service';
import { Observable } from 'rxjs';
import { pluck } from 'rxjs/operators';

@Component({
    selector: 'app-root',
    templateUrl: './app.component.html',
    styleUrls: ['./app.component.css']
})
export class AppComponent implements OnInit, OnDestroy {
    baseUrl: string;
    title = 'Aquarium';
    //temperatures$: Observable<number>;
    constructor(@Inject(ControllerService) public controller: ControllerService) {
        //this.temperatures$ = this.controller.status$.pipe(pluck('CurrentTemperatures'));
    }
    ngOnInit() {
        this.baseUrl = window.location.origin.replace("http:", "ws:") + ":81";
        this.baseUrl = "ws://10.0.0.222:81";
        this.Connect();
    }
    ngOnDestroy() {
    }
    Connect() {
        this.controller.connect(this.baseUrl);
    }
}