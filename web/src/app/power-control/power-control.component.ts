import { Component, Input, Inject, OnInit } from '@angular/core';
import { CurrentPower } from '../port-power';
import { ControllerService } from '../controller.service';
import { Observable } from 'rxjs';
import { FormControl } from '@angular/forms';

@Component({
    selector: 'power-control',
    templateUrl: './power-control.component.html',
    styleUrls: ['./power-control.component.css']
})
export class PowerControlComponent implements OnInit {
    fontPowerMode = new FormControl();
    ports = {
        CurrentPower: [undefined, undefined, undefined],
        ScheduledPower: [undefined, undefined, undefined],
        SelectedPower: [undefined, undefined, undefined],
    };
    portConfig: Array<{ p: string }> = [];
    @Input('portPower') portPower$: Observable<CurrentPower>;
    constructor(@Inject(ControllerService) public controller: ControllerService) { }
    ngOnInit(): void {
        this.portPower$.subscribe(pp => {
            this.portConfig = [];
            if (pp.CurrentPower)
                this.ports.CurrentPower = pp.CurrentPower;
            if (pp.ScheduledPower)
                this.ports.ScheduledPower = pp.ScheduledPower;
            if (pp.SelectedPower)
                this.ports.SelectedPower = pp.SelectedPower;
            this.RefreshPortConfig();
        });
    }
    portChange(p: number, v: string) {
        switch (v) {
            case '0':
                this.ports.SelectedPower[p] = false;
                break;
            case '1':
                this.ports.SelectedPower[p] = true;
                break;
            default:
                this.ports.SelectedPower[p] = null;
        }
        this.controller.Send({ SelectedPower: this.ports.SelectedPower });
        //this.RefreshPortConfig();
    }
    RefreshPortConfig() {
        for (let i in this.ports.SelectedPower) {
            if (this.ports.SelectedPower[i] === false)
                this.portConfig[i] = { p: '0' };
            else if (this.ports.SelectedPower[i] === true)
                this.portConfig[i] = { p: '1' };
            else
                this.portConfig[i] = { p: '2' };
        }
    }
}
